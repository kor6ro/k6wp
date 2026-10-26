//! The K6WP IPC client (task 2.6 / G6).
//!
//! The Studio talks to the engine over a per-session named pipe
//! (`\\.\pipe\k6wp-engine-<session_id>`). This crate is the Rust client: it
//! opens the pipe, VERIFIES the server before it trusts it, sends one
//! request and reads one answer - with a hard timeout on every step.
//!
//! WHY THE VERIFICATION MATTERS
//! A named pipe on Windows can be created by ANY process in the session (and
//! a remote machine can reach `\\host\pipe\name` if the server allows it).
//! If the Studio sends "set_video" or "quit" to whatever is on the other
//! end, an attacker (or a leftover test server) gets to control the user's
//! wallpaper. So the client checks, BEFORE it sends anything:
//!
//!   1. the server's process id (`GetNamedPipeServerProcessId`);
//!   2. the server's session id - it must be the SAME session as the
//!      client (this is what rejects a remote server: the remote process
//!      has no session id in our session);
//!   3. the server's owner - it must be the CURRENT USER;
//!   4. the server's image path - it must be the expected K6WP binary (the
//!      caller passes the expected path; the check can be skipped only
//!      explicitly).
//!
//! Every step is bounded: the connect, the write and the read have their
//! own timeout, so a silent server can never hang the Studio.
//!
//! The message codec itself is NOT here - it is `k6wp_core::ipc` (the frozen
//! contract, replayed against the C++ by the golden fixtures).

use std::ffi::c_void;
use std::path::PathBuf;
use std::time::Duration;

use k6wp_core::ipc::{encode, IpcError as CoreIpcError, IpcMessage};

/// The client's errors.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum IpcError {
    /// The pipe could not be opened (the engine is not running, or the
    /// name is wrong).
    Connect(String),
    /// The server did not pass the verification.
    Unverified(String),
    /// The operation timed out.
    Timeout(String),
    /// The transport failed (the pipe was closed, the write failed, ...).
    Transport(String),
    /// The protocol failed (a malformed answer, a payload over the limit).
    Protocol(String),
}

impl std::fmt::Display for IpcError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            IpcError::Connect(m) => write!(f, "ipc connect failed: {m}"),
            IpcError::Unverified(m) => write!(f, "ipc server not verified: {m}"),
            IpcError::Timeout(m) => write!(f, "ipc timeout: {m}"),
            IpcError::Transport(m) => write!(f, "ipc transport error: {m}"),
            IpcError::Protocol(m) => write!(f, "ipc protocol error: {m}"),
        }
    }
}

impl std::error::Error for IpcError {}

/// The environment variable that overrides the pipe name (the same one the
/// C++ engine reads - E5). A test that starts its own engine uses a unique
/// name so it can never talk to the user's live session.
///
/// **The override is compiled OUT of the release build** (task I4): a
/// shipped binary must not let an environment variable redirect the IPC
/// endpoint - whoever can set an environment variable for the process would
/// then be able to point the app at a pipe it controls. The release
/// binaries are scanned by `tools/check_release_overrides.ps1`.
pub const PIPE_ENV_OVERRIDE: &str = "K6WP_ENGINE_PIPE";

/// The base of the pipe name (`\\.\pipe\k6wp-engine`).
pub const PIPE_NAME_BASE: &str = r"\\.\pipe\k6wp-engine";

/// The image names a client accepts as the engine (task I4).
///
///   - `engine.exe` - the resident engine (today);
///   - `K6WP.exe` - the merged binary (Phase 3: the engine and the launcher
///     become one process).
pub const ACCEPTED_SERVER_IMAGE_NAMES: &[&str] = &["engine.exe", "k6wp.exe"];

/// True when the path is an accepted server image: the file name must be one
/// of the accepted ones, and the directory must be the expected one (the
/// caller passes the full expected path, so the dev path can be allowed
/// only in a debug build - see `dev_server_image_allowed`).
pub fn is_accepted_server_image(actual: &std::path::Path, expected: &std::path::Path) -> bool {
    let Some(name) = actual.file_name() else {
        return false;
    };
    let name = name.to_string_lossy().to_lowercase();
    if !ACCEPTED_SERVER_IMAGE_NAMES.contains(&name.as_str()) {
        return false;
    }
    actual.parent() == expected.parent()
}

/// The DEV image paths (the repository's build directory) are accepted only
/// in a DEBUG build: a release build must talk to the installed engine, not
/// to a developer's build.
pub const fn dev_server_image_allowed() -> bool {
    cfg!(debug_assertions)
}

/// The pipe name for THIS process's session.
///
/// It resolves the session the same way the C++ does
/// (`ProcessIdToSessionId(GetCurrentProcessId())`) and falls back to session
/// 0 when the lookup fails - never empty. An optional `override_name` can be
/// provided for tests; in release builds this parameter is ignored (the override
/// is compiled out via the debug_assertions gate in the caller).
pub fn current_session_pipe_name(override_name: Option<&str>) -> String {
    if let Some(name) = override_name {
        if !name.is_empty() {
            return name.to_string();
        }
    }
    let session = current_session_id();
    format!("{PIPE_NAME_BASE}-{session}")
}

/// The pipe name for a given session id.
pub fn pipe_name_for_session(session_id: u32) -> String {
    format!("{PIPE_NAME_BASE}-{session_id}")
}

/// This process's Terminal Services session id (0 when the lookup fails).
pub fn current_session_id() -> u32 {
    use windows::Win32::System::RemoteDesktop::ProcessIdToSessionId;
    let mut session = 0u32;
    // SAFETY: the out-parameter is a valid, initialised u32.
    unsafe {
        let _ = ProcessIdToSessionId(
            windows::Win32::System::Threading::GetCurrentProcessId(),
            &mut session,
        );
    }
    session
}

/// How the client connects and what it demands from the server.
#[derive(Debug, Clone)]
pub struct ConnectOptions {
    /// The timeout for the connect, the write and the read (each step gets
    /// its own budget).
    pub timeout: Duration,
    /// The image path the server's binary must have. `None` skips the check
    /// - use it only in a test.
    pub expected_server_image: Option<PathBuf>,
    /// The server must run as the current user.
    pub require_current_user: bool,
    /// The server must be in the same Terminal Services session.
    pub require_same_session: bool,
}

impl Default for ConnectOptions {
    fn default() -> Self {
        ConnectOptions {
            timeout: Duration::from_secs(5),
            expected_server_image: None,
            require_current_user: true,
            require_same_session: true,
        }
    }
}

/// What the client learned about the server it is talking to.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ServerIdentity {
    pub process_id: u32,
    pub session_id: u32,
    pub image_path: Option<PathBuf>,
    /// The owner's SID (as a string), when it could be read.
    pub owner_sid: Option<String>,
}

/// A connected client.
pub struct IpcClient {
    handle: windows::Win32::Foundation::HANDLE,
    options: ConnectOptions,
    identity: ServerIdentity,
    /// The pipe this client is connected to (needed by `reconnect`).
    pipe_name: String,
}

impl IpcClient {
    /// Opens the pipe and verifies the server.
    ///
    /// The order matters: the pipe is opened, then the server is verified,
    /// and only then can a request be sent. A failure closes the handle.
    pub fn connect(pipe_name: &str, options: ConnectOptions) -> Result<IpcClient, IpcError> {
        Self::connect_with(pipe_name, options, false)
    }

    /// The connect, with the "the pipe may be momentarily absent" switch.
    ///
    /// `wait_for_the_pipe` is true only for a **reconnect**: the engine
    /// creates the next pipe instance right after the old client left, so
    /// the name can be gone for a moment. A plain `connect` treats a
    /// missing pipe as "the engine is not running" and fails at once.
    fn connect_with(
        pipe_name: &str,
        options: ConnectOptions,
        wait_for_the_pipe: bool,
    ) -> Result<IpcClient, IpcError> {
        let handle = open_pipe(pipe_name, options.timeout, wait_for_the_pipe)?;
        let client = IpcClient {
            handle,
            options,
            identity: ServerIdentity {
                process_id: 0,
                session_id: 0,
                image_path: None,
                owner_sid: None,
            },
            pipe_name: pipe_name.to_string(),
        };
        match client.verify_server() {
            Ok(identity) => {
                let mut client = client;
                client.identity = identity;
                Ok(client)
            }
            Err(e) => Err(e),
        }
    }

    /// Reconnects after a transport failure.
    ///
    /// The old handle is **closed first**: this method consumes `self`, so
    /// the `Drop` impl closes the pipe before the new connect starts. The
    /// engine serves **one client at a time**, so a live old handle keeps
    /// the only pipe instance busy and the new connect would wait for the
    /// whole timeout (`ERROR_PIPE_BUSY`) - the 5 s stall the L2 runs hit.
    /// Releasing the handle first is what makes the reconnect possible at
    /// all (task L4).
    pub fn reconnect(self, options: ConnectOptions) -> Result<IpcClient, IpcError> {
        let pipe_name = self.pipe_name.clone();
        drop(self); // the Drop impl closes the handle
        Self::connect_with(&pipe_name, options, true)
    }

    /// Connects to the engine of THIS session.
    pub fn connect_current_session(options: ConnectOptions) -> Result<IpcClient, IpcError> {
        IpcClient::connect(&current_session_pipe_name(None), options)
    }

    /// The verified identity of the server.
    pub fn server_identity(&self) -> &ServerIdentity {
        &self.identity
    }

    /// Sends a request and reads the answer.
    ///
    /// The payload is bounded by the protocol (64 KiB - the codec refuses a
    /// bigger one), and every step has its own timeout.
    ///
    /// The answer is the RAW ack: the engine answers `{"ok":true}` or
    /// `{"error":"..."}` (and the get_state answer carries the state), NOT a
    /// framed IpcMessage - so the ack is returned as a JSON value and the
    /// caller decides what it means.
    pub fn request(&mut self, message: &IpcMessage) -> Result<serde_json::Value, IpcError> {
        let line = encode(message).map_err(|e: CoreIpcError| IpcError::Protocol(e.to_string()))?;
        self.write_line(line.as_bytes())?;
        let answer = self.read_line()?;
        let text = String::from_utf8_lossy(&answer).to_string();
        let text = text.trim_end_matches(['\r', '\n']);
        serde_json::from_str(text)
            .map_err(|e| IpcError::Protocol(format!("the answer is not JSON: {e}")))
    }

    /// Sends a raw line (the tests use it to send something malformed).
    pub fn write_line(&mut self, bytes: &[u8]) -> Result<(), IpcError> {
        overlapped_write(self.handle, bytes, self.options.timeout)
    }

    /// Reads one message (the trailing newline is stripped).
    pub fn read_line(&mut self) -> Result<Vec<u8>, IpcError> {
        let mut buffer = vec![0u8; k6wp_core::ipc::MAX_PAYLOAD_BYTES + 1024];
        let read = overlapped_read(self.handle, &mut buffer, self.options.timeout)?;
        buffer.truncate(read);
        Ok(buffer)
    }

    /// Closes the pipe.
    pub fn close(self) {
        // The `Drop` impl closes the handle: closing it here as well would
        // close the same handle twice.
        drop(self);
    }

    /// Verifies the server (see the module docs for the list).
    fn verify_server(&self) -> Result<ServerIdentity, IpcError> {
        use windows::Win32::System::Pipes::GetNamedPipeServerProcessId;

        let mut pid = 0u32;
        // SAFETY: the handle is a live pipe handle and the out-parameter is
        // valid.
        let ok = unsafe { GetNamedPipeServerProcessId(self.handle, &mut pid) };
        if ok.is_err() {
            return Err(IpcError::Unverified(
                "the server process id could not be read".to_string(),
            ));
        }
        if pid == 0 {
            return Err(IpcError::Unverified(
                "the server process id is zero".to_string(),
            ));
        }

        let server_session = process_session_id(pid);
        if self.options.require_same_session {
            let own_session = current_session_id();
            if server_session != own_session {
                return Err(IpcError::Unverified(format!(
                    "the server runs in session {server_session}, this client in {own_session}"
                )));
            }
        }

        let owner_sid = process_owner_sid(pid);
        if self.options.require_current_user {
            let own_sid = current_user_sid();
            match (&owner_sid, &own_sid) {
                (Some(a), Some(b)) if a == b => {}
                (None, _) => {
                    return Err(IpcError::Unverified(
                        "the server's owner could not be read".to_string(),
                    ))
                }
                _ => {
                    return Err(IpcError::Unverified(format!(
                        "the server owner is {owner_sid:?}, this client is {own_sid:?}"
                    )))
                }
            }
        }

        let image_path = process_image_path(pid);
        if let Some(expected) = &self.options.expected_server_image {
            match &image_path {
                Some(actual) if paths_equal(actual, expected.as_path()) => {}
                Some(actual) => {
                    return Err(IpcError::Unverified(format!(
                        "the server image is {actual:?}, expected {expected:?}"
                    )))
                }
                None => {
                    return Err(IpcError::Unverified(
                        "the server image path could not be read".to_string(),
                    ))
                }
            }
        }

        Ok(ServerIdentity {
            process_id: pid,
            session_id: server_session,
            image_path,
            owner_sid,
        })
    }
}

impl Drop for IpcClient {
    fn drop(&mut self) {
        unsafe {
            let _ = windows::Win32::Foundation::CloseHandle(self.handle);
        }
    }
}

fn paths_equal(a: &std::path::Path, b: &std::path::Path) -> bool {
    a.to_string_lossy()
        .eq_ignore_ascii_case(&b.to_string_lossy())
}

/// The session id of a process (0 when it cannot be read).
fn process_session_id(pid: u32) -> u32 {
    use windows::Win32::System::RemoteDesktop::ProcessIdToSessionId;
    let mut session = 0u32;
    // SAFETY: the out-parameter is a valid, initialised u32.
    unsafe {
        let _ = ProcessIdToSessionId(pid, &mut session);
    }
    session
}

/// The image path of a process (None when it cannot be read).
fn process_image_path(pid: u32) -> Option<PathBuf> {
    use windows::core::PWSTR;
    use windows::Win32::System::Threading::{
        OpenProcess, QueryFullProcessImageNameW, PROCESS_NAME_FORMAT,
        PROCESS_QUERY_LIMITED_INFORMATION,
    };
    // SAFETY: a limited query right on a live pid; the handle is closed below.
    let process = unsafe { OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid) }.ok()?;
    let mut buffer = [0u16; 32768 / 2];
    let mut size = buffer.len() as u32;
    // SAFETY: the buffer and its length are valid.
    let ok = unsafe {
        QueryFullProcessImageNameW(
            process,
            PROCESS_NAME_FORMAT(0),
            PWSTR(buffer.as_mut_ptr()),
            &mut size,
        )
    };
    unsafe {
        let _ = windows::Win32::Foundation::CloseHandle(process);
    }
    if ok.is_err() {
        return None;
    }
    let text = String::from_utf16_lossy(&buffer[..size as usize]);
    Some(PathBuf::from(text))
}

/// The owner SID and name of a process (None when they cannot be read).
/// The owner SID of a process (None when it cannot be read).
fn process_owner_sid(pid: u32) -> Option<String> {
    use windows::Win32::Foundation::HANDLE;
    use windows::Win32::Security::{GetTokenInformation, TokenUser, TOKEN_QUERY};
    use windows::Win32::System::Threading::{
        OpenProcess, OpenProcessToken, PROCESS_QUERY_LIMITED_INFORMATION,
    };

    // SAFETY: a limited query right on a live pid.
    let process = unsafe { OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid) }.ok()?;
    let mut token = HANDLE::default();
    // SAFETY: the token handle is closed below.
    let opened = unsafe { OpenProcessToken(process, TOKEN_QUERY, &mut token) };
    unsafe {
        let _ = windows::Win32::Foundation::CloseHandle(process);
    }
    if opened.is_err() {
        return None;
    }

    let mut size = 0u32;
    // SAFETY: the first call asks for the size.
    unsafe {
        let _ = GetTokenInformation(token, TokenUser, None, 0, &mut size);
    }
    if size == 0 {
        unsafe {
            let _ = windows::Win32::Foundation::CloseHandle(token);
        }
        return None;
    }
    let mut buffer = vec![0u8; size as usize];
    // SAFETY: the buffer is exactly the size the API asked for.
    let read = unsafe {
        GetTokenInformation(
            token,
            TokenUser,
            Some(buffer.as_mut_ptr() as *mut c_void),
            size,
            &mut size,
        )
    };
    unsafe {
        let _ = windows::Win32::Foundation::CloseHandle(token);
    }
    if read.is_err() {
        return None;
    }
    sid_to_string(&buffer)
}

/// The current user's SID.
fn current_user_sid() -> Option<String> {
    use windows::Win32::Foundation::HANDLE;
    use windows::Win32::Security::{GetTokenInformation, TokenUser, TOKEN_QUERY};
    use windows::Win32::System::Threading::{GetCurrentProcess, OpenProcessToken};

    // SAFETY: the pseudo handle is valid and the token is closed below.
    let process = unsafe { GetCurrentProcess() };
    let mut token = HANDLE::default();
    // SAFETY: the token handle is closed below.
    let opened = unsafe { OpenProcessToken(process, TOKEN_QUERY, &mut token) };
    if opened.is_err() {
        return None;
    }
    let mut size = 0u32;
    unsafe {
        let _ = GetTokenInformation(token, TokenUser, None, 0, &mut size);
    }
    let mut buffer = vec![0u8; size.max(1) as usize];
    let read = unsafe {
        GetTokenInformation(
            token,
            TokenUser,
            Some(buffer.as_mut_ptr() as *mut c_void),
            buffer.len() as u32,
            &mut size,
        )
    };
    unsafe {
        let _ = windows::Win32::Foundation::CloseHandle(token);
    }
    if read.is_err() {
        return None;
    }
    sid_to_string(&buffer)
}

/// The string form of the SID at the start of the buffer.
fn sid_to_string(buffer: &[u8]) -> Option<String> {
    use windows::core::PWSTR;
    use windows::Win32::Foundation::LocalFree;
    use windows::Win32::Security::Authorization::ConvertSidToStringSidW;
    let sid = unsafe {
        let token_user = buffer.as_ptr() as *const windows::Win32::Security::TOKEN_USER;
        (*token_user).User.Sid
    };
    let mut text = PWSTR::null();
    // SAFETY: the SID comes from a live token.
    let ok = unsafe { ConvertSidToStringSidW(sid, &mut text) };
    if ok.is_err() || text.is_null() {
        return None;
    }
    let mut len = 0usize;
    // SAFETY: the string is null-terminated.
    let owned = unsafe {
        while *text.0.add(len) != 0 {
            len += 1;
        }
        let slice = std::slice::from_raw_parts(text.0, len);
        let owned = String::from_utf16_lossy(slice);
        let _ = LocalFree(Some(windows::Win32::Foundation::HLOCAL(
            text.0 as *mut c_void,
        )));
        owned
    };
    Some(owned)
}

/// Opens the pipe (the connect itself is bounded by the timeout).
///
/// A pipe whose instances are ALL busy (the engine serves one client at a
/// time) is not a hard error yet: the client waits for an instance with
/// `WaitNamedPipe`, bounded by the deadline. When the deadline passes, the
/// error says exactly that ("no free instance") instead of a bare Win32
/// code - and the client never hangs (task L4).
///
/// `wait_for_the_pipe` is for the **reconnect** path: the engine creates
/// the next pipe instance right after the old client left, so the NAME can
/// be momentarily absent (the Win32 error 2). A plain `connect` does not
/// wait for that - a missing pipe means the engine is not running, and the
/// caller wants to know that now.
fn open_pipe(
    pipe_name: &str,
    timeout: Duration,
    wait_for_the_pipe: bool,
) -> Result<windows::Win32::Foundation::HANDLE, IpcError> {
    use windows::core::PCWSTR;
    use windows::Win32::Storage::FileSystem::{
        CreateFileW, FILE_FLAG_OVERLAPPED, FILE_GENERIC_READ, FILE_GENERIC_WRITE, FILE_SHARE_MODE,
        OPEN_EXISTING,
    };
    use windows::Win32::System::Pipes::WaitNamedPipeW;

    let wide: Vec<u16> = pipe_name.encode_utf16().chain(std::iter::once(0)).collect();
    let deadline = std::time::Instant::now() + timeout;
    loop {
        // SAFETY: the name is a valid null-terminated wide string and the
        // handle is checked below.
        let handle = unsafe {
            CreateFileW(
                PCWSTR(wide.as_ptr()),
                FILE_GENERIC_READ.0 | FILE_GENERIC_WRITE.0,
                FILE_SHARE_MODE(0),
                None,
                OPEN_EXISTING,
                FILE_FLAG_OVERLAPPED,
                None,
            )
        };
        match handle {
            Ok(h) => {
                // The pipe is in MESSAGE mode (the server created it that
                // way): the read mode must be set on OUR handle too, or a
                // read could return a partial NDJSON line.
                use windows::Win32::System::Pipes::{
                    SetNamedPipeHandleState, NAMED_PIPE_MODE, PIPE_READMODE_MESSAGE,
                };
                let mode = NAMED_PIPE_MODE(PIPE_READMODE_MESSAGE.0);
                // SAFETY: the handle is a live pipe and the out-parameters
                // are valid.
                unsafe {
                    let _ = SetNamedPipeHandleState(
                        h,
                        Some(&mode as *const NAMED_PIPE_MODE),
                        None,
                        None,
                    );
                }
                return Ok(h);
            }
            Err(e) => {
                // The Win32 code is the LOW WORD of the HRESULT: comparing
                // the HRESULT itself with 231 (ERROR_PIPE_BUSY) would never
                // match, and the client would fail instead of waiting for a
                // free pipe instance.
                let last_error = (e.code().0 as u32) & 0xFFFF;
                // ERROR_PIPE_BUSY (231): every instance is in use (the engine
                // is serving another client). ERROR_SEM_TIMEOUT (121): the
                // pipe exists but no instance is free yet.
                // ERROR_FILE_NOT_FOUND (2): the pipe is not there at all.
                let busy = last_error == 231 || last_error == 121;
                let missing = last_error == 2;
                if !busy && !(missing && wait_for_the_pipe) {
                    return Err(IpcError::Connect(format!(
                        "the pipe {pipe_name} could not be opened (the Win32 error {last_error})"
                    )));
                }
                let remaining = deadline.saturating_duration_since(std::time::Instant::now());
                if remaining.is_zero() {
                    return Err(IpcError::Connect(format!(
                        "the pipe {pipe_name} could not be opened within {:.1} s (the Win32 error {last_error}): {}",
                        timeout.as_secs_f64(),
                        if missing {
                            "the engine is not listening"
                        } else {
                            "every instance is in use"
                        }
                    )));
                }
                if busy {
                    // WaitNamedPipe waits for an instance to become free
                    // instead of polling every 50 ms. The wait is bounded
                    // by the deadline, so a busy pipe can never hang the
                    // client. The timeout is clamped to at least 1 ms: a 0
                    // would mean "wait forever" to the Win32 API.
                    let wait_ms = remaining.as_millis().clamp(1, u32::MAX as u128) as u32;
                    // SAFETY: the name is a valid null-terminated wide string.
                    let waited = unsafe { WaitNamedPipeW(PCWSTR(wide.as_ptr()), wait_ms) };
                    if !waited.as_bool() {
                        // SAFETY: GetLastError has no preconditions.
                        let code = unsafe { windows::Win32::Foundation::GetLastError() };
                        return Err(IpcError::Connect(format!(
                            "the pipe {pipe_name} has no free instance after {:.1} s (the Win32 error {}): every instance is in use",
                            timeout.as_secs_f64(),
                            code.0
                        )));
                    }
                    // An instance is free now: open it (the loop retries).
                } else {
                    // The pipe is momentarily absent: the engine is between
                    // two instances. Wait a little and try again.
                    std::thread::sleep(Duration::from_millis(50).min(remaining));
                }
            }
        }
    }
}

/// Writes the bytes with an OVERLAPPED write bounded by the timeout.
fn overlapped_write(
    handle: windows::Win32::Foundation::HANDLE,
    bytes: &[u8],
    timeout: Duration,
) -> Result<(), IpcError> {
    use windows::Win32::Foundation::CloseHandle;
    use windows::Win32::System::Threading::CreateEventW;
    use windows::Win32::System::IO::{GetOverlappedResult, OVERLAPPED};

    let event = unsafe { CreateEventW(None, true, false, None) }
        .map_err(|e| IpcError::Transport(format!("the write event could not be created: {e}")))?;
    let mut overlapped = OVERLAPPED {
        hEvent: event,
        ..Default::default()
    };

    let mut written = 0u32;
    // SAFETY: the buffer is valid for the duration of the call and the
    // OVERLAPPED (with its event) outlives it. The count is requested so a
    // short write can be caught instead of silently truncating the message.
    let ok = unsafe {
        windows::Win32::Storage::FileSystem::WriteFile(
            handle,
            Some(bytes),
            Some(&mut written),
            Some(&mut overlapped),
        )
    };
    if ok.is_ok() {
        unsafe {
            let _ = CloseHandle(event);
        }
        if written as usize != bytes.len() {
            return Err(IpcError::Transport(format!(
                "the write was short: {written} of {} bytes",
                bytes.len()
            )));
        }
        return Ok(());
    }
    // The write is pending: wait for it, bounded.
    let timeout_ms = timeout.as_millis().min(u32::MAX as u128) as u32;
    // SAFETY: the OVERLAPPED and its event are alive until the call returns.
    let waited =
        unsafe { windows::Win32::System::Threading::WaitForSingleObject(event, timeout_ms) };
    let mut transferred = 0u32;
    // SAFETY: the OVERLAPPED and its event are alive until the call returns.
    let done = unsafe {
        GetOverlappedResult(
            handle,
            &overlapped as *const OVERLAPPED,
            &mut transferred,
            false,
        )
    };
    unsafe {
        let _ = CloseHandle(event);
    }
    if waited == windows::Win32::Foundation::WAIT_TIMEOUT {
        return Err(IpcError::Timeout(
            "the write did not finish in time".to_string(),
        ));
    }
    if done.is_ok() {
        return Ok(());
    }
    Err(IpcError::Transport(
        "the write failed on the pipe".to_string(),
    ))
}

/// Reads one message with an OVERLAPPED read bounded by the timeout.
fn overlapped_read(
    handle: windows::Win32::Foundation::HANDLE,
    buffer: &mut [u8],
    timeout: Duration,
) -> Result<usize, IpcError> {
    use windows::Win32::Foundation::{CloseHandle, WAIT_TIMEOUT};
    use windows::Win32::System::Threading::CreateEventW;
    use windows::Win32::System::IO::{GetOverlappedResult, OVERLAPPED};

    let event = unsafe { CreateEventW(None, true, false, None) }
        .map_err(|e| IpcError::Transport(format!("the read event could not be created: {e}")))?;
    let mut overlapped = OVERLAPPED {
        hEvent: event,
        ..Default::default()
    };

    let mut transferred = 0u32;
    // SAFETY: the buffer is valid and the OVERLAPPED (with its event)
    // outlives the call. The byte count MUST be requested: when the read
    // completes SYNCHRONOUSLY (the answer is already in the pipe buffer),
    // `ReadFile` returns TRUE and the count arrives ONLY through
    // `lpNumberOfBytesRead`. Without it the client saw an EMPTY answer
    // ("the answer is not JSON: EOF") and reported a phantom failure -
    // the bug behind the L2 gate's 2/5 lost runs.
    let ok = unsafe {
        windows::Win32::Storage::FileSystem::ReadFile(
            handle,
            Some(&mut buffer[..]),
            Some(&mut transferred),
            Some(&mut overlapped),
        )
    };
    if ok.is_ok() {
        unsafe {
            let _ = CloseHandle(event);
        }
        return Ok(transferred as usize);
    }
    let timeout_ms = timeout.as_millis().min(u32::MAX as u128) as u32;
    // SAFETY: the OVERLAPPED and its event are alive until the call returns.
    let waited =
        unsafe { windows::Win32::System::Threading::WaitForSingleObject(event, timeout_ms) };
    // SAFETY: the OVERLAPPED and its event are alive until the call returns.
    let done = unsafe {
        GetOverlappedResult(
            handle,
            &overlapped as *const OVERLAPPED,
            &mut transferred,
            false,
        )
    };
    unsafe {
        let _ = CloseHandle(event);
    }
    if waited == WAIT_TIMEOUT {
        return Err(IpcError::Timeout(
            "the answer did not arrive in time".to_string(),
        ));
    }
    if done.is_ok() {
        return Ok(transferred as usize);
    }
    Err(IpcError::Transport(
        "the read failed on the pipe".to_string(),
    ))
}

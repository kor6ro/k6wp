import re

with open("rust/crates/k6wp-studio/src/worker.rs", "r") as f:
    content = f.read()

# Replace the impl Sink block - remove test functions from inside impl Sink
old_impl = """impl Sink {
    fn emit(&self, event: Event) {
        if let Ok(callback) = self.0.lock() {
            callback(event);
        }
    }

    #[test]
    fn import_validates_extension() {
        // Create a temp file with invalid extension
        let temp = std::env::temp_dir().join(format!("k6wp-test-import-{}.txt", std::process::id()));
        std::fs::write(&temp, b"fake content").expect("must create temp file");
        let cache = std::env::temp_dir().join("k6wp-test-thumb-import-ext");
        std::fs::create_dir_all(&cache).ok();
        let result = generate_thumbnail(&temp, &cache, THUMB_WIDTH, THUMB_HEIGHT);
        assert!(matches!(result, Err(ThumbError::InvalidMedia(_))), "txt should be rejected");
    }

    #[test]
    fn import_validates_missing_file() {
        let missing = PathBuf::from(r"Z:\\definitely\\not\\here.mp4");
        let cache = std::env::temp_dir().join("k6wp-test-thumb-missing");
        std::fs::create_dir_all(&cache).ok();
        let result = generate_thumbnail(&missing, &cache, THUMB_WIDTH, THUMB_HEIGHT);
        assert!(matches!(result, Err(ThumbError::InvalidMedia(_))), "missing file should be rejected");
    }
}

/// Starts the worker thread. The callback runs on the worker thread; the"""

new_impl = """impl Sink {
    fn emit(&self, event: Event) {
        if let Ok(callback) = self.0.lock() {
            callback(event);
        }
    }
}

/// Starts the worker thread. The callback runs on the worker thread; the"""

content = content.replace(old_impl, new_impl)

with open("rust/crates/k6wp-studio/src/worker.rs", "w") as f:
    f.write(content)

print("Done")

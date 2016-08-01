# Carbon Changelog

### 0.1 — Unreleased

- Fixed backgrounded applications on iOS potentially crashing due to not flushing the GPU command queue
- Fixed decoding of UTF-16 surrogate pairs in `fromUTF16()`

### 0.1-alpha-e6765f1 — July 15, 2016

- Added support for Xcode 7.3
- Improved support for full-resolution rendering on the iPhone 6+
- The `GUIWindow::initialize()` methods now take the initial window position as a `Vec2` rather than as separate
  parameters
- Report more detailed errors from `ScrollingLayer` when invalid tile indices are specified
- Upgraded dependency versions

### 0.1-alpha-7daaf59 — December 23, 2015

- Uploaded project to GitHub

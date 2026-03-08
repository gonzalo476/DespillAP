#pragma once

/**
 * @file pixel_ops.h
 * @brief Lightweight pixel-access utilities for Nuke Iop plugins.
 *
 * No external dependencies beyond the Nuke DDImage SDK and C++17 STL.
 *
 * ## Overview
 *
 * Nuke's `engine()` callback provides raw `float*` row pointers per channel.
 * Managing those pointers — especially across multiple inputs — leads to
 * repetitive, error-prone setup code in every plugin.
 *
 * This header provides three thin abstractions that eliminate that boilerplate:
 *
 * - `pixel::RowReader`   — reads one or more channels from a `DD::Image::Row`
 * - `pixel::RowWriter`   — writes one or more channels into a `DD::Image::Row`
 * - `pixel::PixelCursor` — combines read + write pointers and advances them together
 *
 * ## Typical engine() usage
 *
 * @code
 * void MyIop::engine(int y, int x, int r, ChannelMask channels, Row& row)
 * {
 *     row.get(input0(), y, x, r, Mask_RGB);
 *
 *     pixel::PixelCursor px(row, x);
 *     px.addReadChannel(Chan_Red);
 *     px.addReadChannel(Chan_Green);
 *     px.addReadChannel(Chan_Blue);
 *     px.addWriteChannel(Chan_Red);
 *     px.addWriteChannel(Chan_Green);
 *     px.addWriteChannel(Chan_Blue);
 *
 *     for (int col = x; col < r; ++col, px.advance()) {
 *         float r = px.read(0), g = px.read(1), b = px.read(2);
 *         px.write(0, r * 0.9f);
 *         px.write(1, g * 0.9f);
 *         px.write(2, b * 0.9f);
 *     }
 * }
 * @endcode
 *
 * ## Channel index helpers
 *
 * `pixel::rgbChannels()` returns `{Chan_Red, Chan_Green, Chan_Blue}` — use it
 * to set up RGB cursors without spelling out each channel manually.
 *
 * ## Math helpers
 *
 * - `pixel::clamp01(v)`   — saturate to [0, 1]
 * - `pixel::lerp(a,b,t)`  — linear interpolation
 * - `pixel::luma709(r,g,b)` — Rec.709 luminance
 * - `pixel::smoothstep(edge0, edge1, x)` — smooth Hermite step
 *
 * @note All classes operate directly on Nuke's existing `Row` memory.
 *       No additional allocations are made. Copy semantics are intentionally
 *       deleted to prevent accidental misuse across scanlines.
 */

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <initializer_list>

// Nuke SDK headers (must be on include path)
#include "DDImage/Channel.h"
#include "DDImage/Row.h"

namespace pixel
{

  // =============================================================================
  // Constants
  // =============================================================================

  /// Maximum number of channels a RowReader / RowWriter / PixelCursor can hold.
  /// Increase if you need more than 8 simultaneous channels (rare in practice).
  static constexpr int kMaxChannels = 8;

  // =============================================================================
  // Channel helpers
  // =============================================================================

  /**
 * @brief Returns the three RGB channels in R, G, B order.
 *
 * Convenience wrapper so you don't have to spell out each channel when
 * building a cursor for a standard RGB operation.
 *
 * @code
 * for (auto ch : pixel::rgbChannels()) { cursor.addReadChannel(ch); }
 * @endcode
 */
  inline std::array<DD::Image::Channel, 3> rgbChannels()
  {
    return {DD::Image::Chan_Red, DD::Image::Chan_Green, DD::Image::Chan_Blue};
  }

  /**
 * @brief Returns the four RGBA channels in R, G, B, A order.
 *
 * @code
 * for (auto ch : pixel::rgbaChannels()) { cursor.addReadChannel(ch); }
 * @endcode
 */
  inline std::array<DD::Image::Channel, 4> rgbaChannels()
  {
    return {DD::Image::Chan_Red, DD::Image::Chan_Green, DD::Image::Chan_Blue,
            DD::Image::Chan_Alpha};
  }

  // =============================================================================
  // Math helpers
  // =============================================================================

  /**
 * @brief Saturates `v` to the range [0, 1].
 *
 * Equivalent to Nuke SDK's `clamp()` but scoped to avoid name collisions
 * with the Nuke global `clamp` function.
 *
 * @param v Input value
 * @return Value clamped to [0, 1]
 */
  inline float clamp01(float v) noexcept
  {
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
  }

  /**
 * @brief Clamps `v` to the range [lo, hi].
 *
 * @param v  Input value
 * @param lo Lower bound (inclusive)
 * @param hi Upper bound (inclusive)
 * @return Value clamped to [lo, hi]
 */
  inline float clampf(float v, float lo, float hi) noexcept
  {
    return v < lo ? lo : (v > hi ? hi : v);
  }

  /**
 * @brief Linear interpolation between `a` and `b` by factor `t`.
 *
 * `t = 0` returns `a`; `t = 1` returns `b`. `t` is not clamped — pass
 * `clamp01(t)` explicitly if needed.
 *
 * @param a Start value
 * @param b End value
 * @param t Blend factor
 * @return `a + t * (b - a)`
 */
  inline float lerp(float a, float b, float t) noexcept
  {
    return a + t * (b - a);
  }

  /**
 * @brief Smooth Hermite interpolation (Ken Perlin's smoothstep).
 *
 * Returns 0 for `x <= edge0`, 1 for `x >= edge1`, and a smooth curve
 * in between. Equivalent to GLSL `smoothstep`.
 *
 * @param edge0 Lower edge (maps to 0)
 * @param edge1 Upper edge (maps to 1)
 * @param x     Input value
 * @return Smoothly interpolated value in [0, 1]
 */
  inline float smoothstep(float edge0, float edge1, float x) noexcept
  {
    const float t = clamp01((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
  }

  /**
 * @brief Rec.709 luminance from linear RGB values.
 *
 * Standard weights used in HD/digital cinema pipelines.
 * Pass linear-light float values (not gamma-encoded).
 *
 * @param r Red channel value
 * @param g Green channel value
 * @param b Blue channel value
 * @return Perceptual luminance in the same range as the inputs
 */
  inline float luma709(float r, float g, float b) noexcept
  {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
  }

  /**
 * @brief Rec.601 (SD) luminance from linear RGB values.
 *
 * Use when working with SD broadcast content or matching legacy tools.
 *
 * @param r Red channel value
 * @param g Green channel value
 * @param b Blue channel value
 * @return Perceptual luminance
 */
  inline float luma601(float r, float g, float b) noexcept
  {
    return 0.299f * r + 0.587f * g + 0.114f * b;
  }

  /**
 * @brief Average (equal-weight) luminance from RGB.
 *
 * Simple arithmetic mean. Useful for despill and matte operations
 * where perceptual weighting is not required.
 *
 * @param r Red channel value
 * @param g Green channel value
 * @param b Blue channel value
 * @return `(r + g + b) / 3`
 */
  inline float lumaAverage(float r, float g, float b) noexcept
  {
    return (r + g + b) * (1.0f / 3.0f);
  }

  // =============================================================================
  // RowReader
  // =============================================================================

  /**
 * @brief Read-only access to up to `kMaxChannels` channels of a Nuke `Row`.
 *
 * Stores a `const float*` pointer per registered channel, all offset to
 * column `x`. Call `advance()` each iteration to move all pointers forward
 * by one pixel.
 *
 * ## Example
 * @code
 * pixel::RowReader rd(row, x);
 * rd.add(Chan_Red); rd.add(Chan_Green); rd.add(Chan_Blue);
 *
 * for (int col = x; col < r; ++col, rd.advance()) {
 *     float red   = rd[0];
 *     float green = rd[1];
 *     float blue  = rd[2];
 *     // ...
 * }
 * @endcode
 *
 * @note Instances are non-copyable to prevent accidental sharing of pointer
 *       state between scanlines.
 */
  class RowReader
  {
   public:
    /**
     * @brief Constructs a reader bound to a row starting at column `x`.
     *
     * @param row Source Nuke row
     * @param x   Starting column (same `x` passed to `engine()`)
     */
    RowReader(const DD::Image::Row& row, int x) : row_(row), startX_(x), count_(0) {}

    RowReader(const RowReader&) = delete;
    RowReader& operator=(const RowReader&) = delete;

    /**
     * @brief Registers a channel for reading.
     *
     * The channel's pointer is captured at construction time (offset by `x`).
     * Index in subsequent `operator[]` calls corresponds to registration order.
     *
     * @param ch Channel to register (e.g. `Chan_Red`)
     * @return Index assigned to this channel
     */
    int add(DD::Image::Channel ch)
    {
      assert(count_ < kMaxChannels && "RowReader: too many channels");
      ptrs_[count_] = row_[ch] + startX_;
      return count_++;
    }

    /**
     * @brief Returns the current value of the channel at `index`.
     *
     * `index` is the value returned by (or the insertion order of) `add()`.
     * No bounds checking in release builds.
     *
     * @param index Zero-based channel index
     * @return Float value at the current pixel column
     */
    float operator[](int index) const noexcept
    {
      assert(index >= 0 && index < count_);
      return *ptrs_[index];
    }

    /**
     * @brief Advances all channel pointers forward by one pixel.
     *
     * Call once per loop iteration, after reading all channel values.
     */
    void advance() noexcept
    {
      for(int i = 0; i < count_; ++i) ++ptrs_[i];
    }

    /// Returns the number of registered channels.
    int channelCount() const noexcept { return count_; }

   private:
    const DD::Image::Row& row_;
    int startX_;
    int count_;
    const float* ptrs_[kMaxChannels] = {};
  };

  // =============================================================================
  // RowWriter
  // =============================================================================

  /**
 * @brief Write access to up to `kMaxChannels` channels of a Nuke `Row`.
 *
 * Calls `row.writable(ch)` for each registered channel and stores
 * the resulting `float*` offset to column `x`.
 *
 * ## Example
 * @code
 * pixel::RowWriter wr(row, x);
 * wr.add(Chan_Red); wr.add(Chan_Green); wr.add(Chan_Blue);
 *
 * for (int col = x; col < r; ++col, wr.advance()) {
 *     wr.set(0, outR);
 *     wr.set(1, outG);
 *     wr.set(2, outB);
 * }
 * @endcode
 *
 * @note Instances are non-copyable.
 */
  class RowWriter
  {
   public:
    /**
     * @brief Constructs a writer bound to a row starting at column `x`.
     *
     * @param row Target Nuke row (non-const; writable() will be called)
     * @param x   Starting column
     */
    RowWriter(DD::Image::Row& row, int x) : row_(row), startX_(x), count_(0) {}

    RowWriter(const RowWriter&) = delete;
    RowWriter& operator=(const RowWriter&) = delete;

    /**
     * @brief Registers a channel for writing.
     *
     * Calls `row.writable(ch)` immediately to obtain the mutable pointer.
     *
     * @param ch Channel to register
     * @return Index assigned to this channel
     */
    int add(DD::Image::Channel ch)
    {
      assert(count_ < kMaxChannels && "RowWriter: too many channels");
      ptrs_[count_] = row_.writable(ch) + startX_;
      return count_++;
    }

    /**
     * @brief Writes `value` to the channel at `index` for the current pixel.
     *
     * @param index Zero-based channel index (registration order)
     * @param value Value to write
     */
    void set(int index, float value) noexcept
    {
      assert(index >= 0 && index < count_);
      *ptrs_[index] = value;
    }

    /**
     * @brief Returns a writable reference to the channel at `index`.
     *
     * Allows compound assignment: `wr[0] += delta;`
     *
     * @param index Zero-based channel index
     * @return Reference to the current pixel's float value
     */
    float& operator[](int index) noexcept
    {
      assert(index >= 0 && index < count_);
      return *ptrs_[index];
    }

    /**
     * @brief Advances all channel pointers forward by one pixel.
     *
     * Call once per loop iteration, after writing all channel values.
     */
    void advance() noexcept
    {
      for(int i = 0; i < count_; ++i) ++ptrs_[i];
    }

    /// Returns the number of registered channels.
    int channelCount() const noexcept { return count_; }

   private:
    DD::Image::Row& row_;
    int startX_;
    int count_;
    float* ptrs_[kMaxChannels] = {};
  };

  // =============================================================================
  // PixelCursor
  // =============================================================================

  /**
 * @brief Combined read + write cursor over one or more channels of a Nuke Row.
 *
 * Unifies `RowReader` and `RowWriter` into a single object that keeps
 * read and write pointers in sync. Most plugins only need one cursor per
 * primary input, making this the recommended entry point.
 *
 * Read channels and write channels are registered independently via
 * `addReadChannel()` / `addWriteChannel()`, so you can have asymmetric
 * sets (e.g. read RGBA, write only RGB).
 *
 * ## Full example — despill-style plugin
 * @code
 * void MyDespill::ProcessCPU(int y, int x, int r, ChannelMask channels, Row& row)
 * {
 *     row.get(input0(), y, x, r, Mask_RGB);
 *
 *     // Optional secondary inputs
 *     Row limitRow(x, r);
 *     if (input(1)) limitRow.get(*input(1), y, x, r, Mask_Alpha);
 *
 *     pixel::PixelCursor px(row, x);
 *     for (auto ch : pixel::rgbChannels()) {
 *         px.addReadChannel(ch);
 *         px.addWriteChannel(ch);
 *     }
 *
 *     pixel::RowReader limitRd(limitRow, x);
 *     limitRd.add(Chan_Alpha);
 *
 *     for (int col = x; col < r; ++col, px.advance(), limitRd.advance()) {
 *         float r = px.read(0), g = px.read(1), b = px.read(2);
 *         float limit = limitRd[0];
 *
 *         // ... process ...
 *
 *         px.write(0, outR);
 *         px.write(1, outG);
 *         px.write(2, outB);
 *     }
 * }
 * @endcode
 *
 * @note Non-copyable.
 */
  class PixelCursor
  {
   public:
    /**
     * @brief Constructs a cursor bound to a row starting at column `x`.
     *
     * @param row The Nuke row (used for both read and write access)
     * @param x   Starting column
     */
    PixelCursor(DD::Image::Row& row, int x) : row_(row), startX_(x), readCount_(0), writeCount_(0)
    {
    }

    PixelCursor(const PixelCursor&) = delete;
    PixelCursor& operator=(const PixelCursor&) = delete;

    // ── Channel registration ──────────────────────────────────────────────────

    /**
     * @brief Registers a channel for reading.
     *
     * @param ch Channel to read (e.g. `Chan_Green`)
     * @return Read index for use with `read()`
     */
    int addReadChannel(DD::Image::Channel ch)
    {
      assert(readCount_ < kMaxChannels && "PixelCursor: too many read channels");
      rPtrs_[readCount_] = row_[ch] + startX_;
      return readCount_++;
    }

    /**
     * @brief Registers a channel for writing.
     *
     * Internally calls `row.writable(ch)`.
     *
     * @param ch Channel to write (e.g. `Chan_Green`)
     * @return Write index for use with `write()` / `writeRef()`
     */
    int addWriteChannel(DD::Image::Channel ch)
    {
      assert(writeCount_ < kMaxChannels && "PixelCursor: too many write channels");
      wPtrs_[writeCount_] = row_.writable(ch) + startX_;
      return writeCount_++;
    }

    // ── Per-pixel access ──────────────────────────────────────────────────────

    /**
     * @brief Returns the current value of a read channel.
     *
     * @param index Read index (registration order via `addReadChannel`)
     * @return Float value at the current pixel column
     */
    float read(int index) const noexcept
    {
      assert(index >= 0 && index < readCount_);
      return *rPtrs_[index];
    }

    /**
     * @brief Writes a value to a write channel.
     *
     * @param index Write index (registration order via `addWriteChannel`)
     * @param value Value to write
     */
    void write(int index, float value) noexcept
    {
      assert(index >= 0 && index < writeCount_);
      *wPtrs_[index] = value;
    }

    /**
     * @brief Returns a writable reference to a write channel.
     *
     * Allows compound assignment: `px.writeRef(0) += delta;`
     *
     * @param index Write index
     * @return Reference to the current pixel's mutable float
     */
    float& writeRef(int index) noexcept
    {
      assert(index >= 0 && index < writeCount_);
      return *wPtrs_[index];
    }

    /**
     * @brief Advances all read and write pointers by one pixel.
     *
     * Call at the end of each loop iteration (or in the loop increment).
     *
     * @code
     * for (int col = x; col < r; ++col, px.advance()) { ... }
     * @endcode
     */
    void advance() noexcept
    {
      for(int i = 0; i < readCount_; ++i) ++rPtrs_[i];
      for(int i = 0; i < writeCount_; ++i) ++wPtrs_[i];
    }

    // ── Convenience: pass-through ─────────────────────────────────────────────

    /**
     * @brief Copies the current read value of channel `i` directly to write channel `i`.
     *
     * Shorthand for `write(i, read(i))`. Assumes matching read/write
     * channel counts and order. Useful for pass-through in channels
     * that are not being processed this frame.
     *
     * @param index Channel index (must be valid for both read and write)
     */
    void passThrough(int index) noexcept { write(index, read(index)); }

    /**
     * @brief Passes through all registered read channels to the matching write channels.
     *
     * Useful at the start of engine() to copy channels you'll overwrite
     * later, or at the end to flush unmodified channels.
     *
     * Requires `readCount_ == writeCount_` — asserts in debug builds.
     */
    void passThroughAll() noexcept
    {
      assert(readCount_ == writeCount_ && "passThroughAll: read/write channel counts must match");
      for(int i = 0; i < readCount_; ++i) *wPtrs_[i] = *rPtrs_[i];
    }

    // ── Properties ────────────────────────────────────────────────────────────

    /// Returns the number of registered read channels.
    int readChannelCount() const noexcept { return readCount_; }

    /// Returns the number of registered write channels.
    int writeChannelCount() const noexcept { return writeCount_; }

   private:
    DD::Image::Row& row_;
    int startX_;
    int readCount_;
    int writeCount_;
    const float* rPtrs_[kMaxChannels] = {};
    float* wPtrs_[kMaxChannels] = {};
  };

  // =============================================================================
  // MultiInputReader
  // =============================================================================

  /**
 * @brief Reads a single channel from up to `kMaxChannels` independent Rows.
 *
 * Typical use: reading the same channel (e.g. Chan_Alpha) from multiple
 * inputs (source, limit matte, colour reference) in a single loop without
 * managing separate pointer arrays.
 *
 * ## Example
 * @code
 * pixel::MultiInputReader masks(x);
 * masks.add(limitRow,  Chan_Alpha);   // index 0
 * masks.add(respillRow, Chan_Red);    // index 1
 *
 * for (int col = x; col < r; ++col, masks.advance()) {
 *     float limit   = masks[0];
 *     float respill = masks[1];
 *     // ...
 * }
 * @endcode
 *
 * @note Non-copyable.
 */
  class MultiInputReader
  {
   public:
    /**
     * @brief Constructs a reader starting at column `x`.
     * @param x Starting column (same `x` as in `engine()`)
     */
    explicit MultiInputReader(int x) : startX_(x), count_(0) {}

    MultiInputReader(const MultiInputReader&) = delete;
    MultiInputReader& operator=(const MultiInputReader&) = delete;

    /**
     * @brief Registers one channel from a given row.
     *
     * @param row Row to read from (must outlive this object)
     * @param ch  Channel to read
     * @return Index assigned to this entry
     */
    int add(const DD::Image::Row& row, DD::Image::Channel ch)
    {
      assert(count_ < kMaxChannels && "MultiInputReader: too many inputs");
      ptrs_[count_] = row[ch] + startX_;
      return count_++;
    }

    /**
     * @brief Returns the current value for the entry at `index`.
     * @param index Zero-based registration index
     * @return Float value at the current pixel column
     */
    float operator[](int index) const noexcept
    {
      assert(index >= 0 && index < count_);
      return *ptrs_[index];
    }

    /**
     * @brief Advances all pointers by one pixel.
     */
    void advance() noexcept
    {
      for(int i = 0; i < count_; ++i) ++ptrs_[i];
    }

    /// Returns the number of registered input channels.
    int count() const noexcept { return count_; }

   private:
    int startX_;
    int count_;
    const float* ptrs_[kMaxChannels] = {};
  };

}  // namespace pixel
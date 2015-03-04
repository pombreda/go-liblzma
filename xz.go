package xz

// #cgo LDFLAGS: -llzma
// #include <lzma.h>
import "C"

import (
    "errors"
    "io"
    "fmt"
    "math"
    "runtime"
)

// Reader for compressed files in xz format.
//
// This reader does internal buffering, meaning that it's not suitable for xz
// streams embedded in other files (it may read beyond the end of the xz part).
type Reader struct {
    action C.lzma_action
    buf []byte
    r io.Reader
    stream C.lzma_stream
}

func NewReader(r io.Reader) (*Reader, error) {
    var xz Reader
    xz.buf = make([]byte, 512)
    if err := xz.Reset(r); err != nil {
        return nil, err
    }
    return &xz, nil
}

func (r *Reader) Read(p []byte) (n int, err error) {
    var rderr error
    buf := r.buf
    strm := &r.stream

    strm.next_out = (*C.uint8_t)(&p[0])
    strm.avail_out = C.size_t(len(p))

    for strm.avail_out > 0 {
        if strm.avail_in == 0 {
            var nread int

            buf = r.buf[:cap(r.buf)]
            nread, rderr = r.r.Read(buf)
            r.buf = buf[:nread]
            strm.avail_in = C.size_t(nread)
            if nread == 0 {
                strm.next_in = nil
            } else {
                strm.next_in = (*C.uint8_t)(&buf[0])
            }

            if rderr == io.EOF {
                r.action = C.LZMA_FINISH
            } else if rderr != nil {
                err = rderr
            }
        }

        state := C.lzma_code(strm, r.action)

        if state == C.LZMA_STREAM_END {
            err = io.EOF
            break
        } else if state != C.LZMA_OK {
            err = decodeError(state)
            break
        }
    }
    n = len(p) - int(strm.avail_out)
    return
}

// Reset reader so that it can be reused to decompress another stream.
func (xz *Reader) Reset(r io.Reader) error {
    xz.action = C.LZMA_RUN
    xz.buf = xz.buf[:0]
    xz.r = r
    e := C.lzma_stream_decoder(&xz.stream, math.MaxUint64, C.LZMA_CONCATENATED)
    runtime.SetFinalizer(&xz, end)
    if err := decodeError(e); err != nil {
        return err
    }

    xz.stream.avail_in = 0
    xz.stream.next_in = nil

    return nil
}

func end(xz *Reader) {
    C.lzma_end(&xz.stream)
}

// Returns the total number of compressed bytes consumed.
func (xz *Reader) TotalIn() uint64 {
    return uint64(xz.stream.total_in)
}

// Returns the total number of bytes produced by decompression.
func (xz *Reader) TotalOut() uint64 {
    return uint64(xz.stream.total_out)
}

var (
    // Descriptions taken from lzma/base.h.
    NoCheck = errors.New("lzma: Input stream has no integrity check")
    UnsupportedCheck = errors.New("lzma: Cannot calculate the integrity check")
    MemError = errors.New("lzma: Cannot allocate memory")
    MemlimitError = errors.New("lzma: Memory usage limit was reached")
    FormatError = errors.New("lzma: File format not recognized")
    OptionsError = errors.New("lzma: Invalid or unsupported options")
    DataError = errors.New("lzma: Data is corrupt")
    BufError = errors.New("lzma: No progress is possible")
    ProgError = errors.New("lzma: Programming error")
)

func decodeError(code C.lzma_ret) error {
    switch code {
    case C.LZMA_OK:
        return nil
    case C.LZMA_STREAM_END:
        return io.EOF
    case C.LZMA_NO_CHECK:
        return NoCheck
    case C.LZMA_UNSUPPORTED_CHECK:
        return UnsupportedCheck
    case C.LZMA_MEM_ERROR:
        return MemError
    case C.LZMA_FORMAT_ERROR:
        return FormatError
    case C.LZMA_OPTIONS_ERROR:
        return OptionsError
    case C.LZMA_DATA_ERROR:
        return DataError
    case C.LZMA_BUF_ERROR:
        return BufError
    case C.LZMA_PROG_ERROR:
        panic(ProgError)
    }
    panic(fmt.Sprintf("lzma: unknown error code %d", int(code)))
}

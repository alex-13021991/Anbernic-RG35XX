const fs = require('node:fs');
const path = require('node:path');
const { WASI } = require('node:wasi');
(async () => {
  const env=process.argv[4]?{SYSINFO_FIXTURE:process.argv[4]}:{};
  const wasi = new WASI({ version:'preview1', args:[], env, preopens:{'/fixtures':path.resolve(process.argv[3])} });
  const wasm = await WebAssembly.compile(fs.readFileSync(process.argv[2]));
  let instance;
  const wasiImports = {...wasi.wasiImport};
  // Node/libuv on Windows lacks WASI fd_readdir. Supply that one operation
  // from the same real fixture tree, leaving stat/open/unlink to WASI.
  const paths = new Map([[3,path.resolve(process.argv[3])]]);
  wasiImports.path_open = (...args) => {
    const [fd,flags,pointer,length,,,,,opened] = args;
    const relative = Buffer.from(instance.exports.memory.buffer,pointer,length).toString();
    const result = wasi.wasiImport.path_open(...args);
    if(result===0 && paths.has(fd)) paths.set(new DataView(instance.exports.memory.buffer).getUint32(opened,true),path.resolve(paths.get(fd),relative));
    return result;
  };
  wasiImports.fd_readdir = (fd,buffer,length,cookie,used) => {
    if(!paths.has(fd))return 8;
    try {
      const entries=fs.readdirSync(paths.get(fd),{withFileTypes:true});
      let offset=0;
      const memory=Buffer.from(instance.exports.memory.buffer);
      for(let i=Number(cookie);i<entries.length;i++) {
        const entry=entries[i],name=Buffer.from(entry.name),record=Buffer.alloc(24+name.length);
        record.writeBigUInt64LE(BigInt(i+1),0);
        record.writeBigUInt64LE(fs.lstatSync(path.join(paths.get(fd),entry.name),{bigint:true}).ino,8);
        record.writeUInt32LE(name.length,16);record[20]=entry.isDirectory()?3:entry.isSymbolicLink()?7:4;name.copy(record,24);
        const take=Math.min(record.length,length-offset);record.copy(memory,buffer+offset,0,take);offset+=take;
        if(offset===length)break;
      }
      memory.writeUInt32LE(offset,used);return 0;
    }catch(e){console.error('readdir fixture',e.message);return 29;}
  };
  instance = await WebAssembly.instantiate(wasm, { wasi_snapshot_preview1: wasiImports });
  wasi.start(instance);
})();

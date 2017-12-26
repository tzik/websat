
export async function loadWasm(url) {
  let buf = readbuffer(url);
  return WebAssembly.compile(buf);
}

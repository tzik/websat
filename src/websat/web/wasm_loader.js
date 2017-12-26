
export async function loadWasm(url) {
  let response = fetch(url);
  try {
    return WebAssembly.compileStreaming(response);
  } catch (e) {
    if (!(e instanceof ReferenceError)) {
      throw e;
    }
  }
  let res = await response;
  let buf = await res.arrayBuffer();
  return WebAssembly.compile(buf);
}

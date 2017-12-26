
import {loadWasm} from "./wasm_loader.js";
import {encodeUTF8, decodeUTF8} from "./text_decoder.js";

function readText(memory, address, length) {
  return decodeUTF8(new Uint8Array(memory.buffer, address, length));
}

export async function loadSolver() {
  let module = await loadWasm('websat.wasm');
  let memory = new WebAssembly.Memory({initial: 2});
  let env = {
    memory: memory,
    pow: Math.pow,
    print: (address, length) => print(readText(memory, address, length)),
    throwError: (msg, msg_len, filename, filename_len, lineno) => {
      throw new Error();
      throw new Error(readText(memory, msg, msg_len),
                      readText(memory, filename, filename_len),
                      lineno);
    }
  };
  let instance = await WebAssembly.instantiate(module, {env});
  return new WebSAT(memory, instance);
}

export class WebSAT {
  constructor(memory, solver) {
    this.memory = memory;
    this.solver = solver;
    this.solver.exports.init();
  }

  newLiteral() {
    return this.solver.exports.newLiteral();
  }

  addClause(...literals) {
    let length = literals.length;
    let address = this.solver.exports.malloc(length * 4);
    if (address === 0) {
      throw new Error("OOM");
    }

    let buf = new Int32Array(this.memory.buffer, address, length);
    buf.set(literals);
    buf = null;

    this.solver.exports.addClause(address, length);
    this.solver.exports.free(address);
  }

  solve() {
    return this.solver.exports.solve();
  }

  extract() {
    let length = this.solver.exports.getNVars();
    let address = this.solver.exports.malloc(length * 4);
    if (address === 0) {
      throw new Error("OOM");
    }

    let m = ['true', 'false', 'undef'];
    let res = ['undef'];
    this.solver.exports.extract(address, length);
    let buf = new Uint8Array(this.memory.buffer, address, length);
    for (let v of buf) {
      res.push(m[v]);
    }
    buf = null;
    this.solver.exports.free(address);
    return res;
  }

  reset() {
    this.solver.exports.reset();
  }
}

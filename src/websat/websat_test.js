
import {loadSolver} from "./websat.js";

testRunner.waitUntilDone();

(async (binary, ...args) => {
  let solver = await loadSolver();
  let x = solver.newLiteral();
  solver.addClause(x);
  print(solver.solve());
  print(solver.extract());
})(...arguments).catch(e => {
  if (e instanceof Error) {
    print(e.stack);
  } else {
    print(e);
  }
}).finally(() => {
  testRunner.notifyDone();
});

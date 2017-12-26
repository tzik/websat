
import {loadSolver} from "./websat.js";

testRunner.waitUntilDone();

function exclusive(solver, xs) {
  solver.addClause(...xs);
  for (let i = 0; i < xs.length; ++i) {
    for (let j = i + 1; j < xs.length; ++j) {
      solver.addClause(-xs[i], -xs[j]);
    }
  }
}

(async () => {
  let solver = await loadSolver();
  let x = solver.newLiteral();
  solver.addClause(x);

  let n = 3;
  let range = Array.from(new Array(n * n).keys());
  let field = [];

  for (let i of range) {
    let row = [];
    for (let j of range) {
      let cell = [];
      for (let k of range) {
        cell.push(solver.newLiteral());
      }
      row.push(cell);
    }
    field.push(row);
  }

  for (let i of range) {
    for (let j of range) {
      exclusive(solver, range.map(k => field[i][j][k]));
      exclusive(solver, range.map(k => field[j][k][i]));
      exclusive(solver, range.map(k => field[k][i][j]));

      let y = n * (j / n | 0);
      let x = n * (j % n);
      exclusive(solver, range.map(k => field[y + (k / n | 0)][x + k % n][i]));
    }
  }

  // https://ja.wikipedia.org/wiki/%E6%95%B0%E7%8B%AC
  let input = `53  7    
6  195   
 98    6 
8   6   3
4  8 3  1
7   2   6
 6    28 
   419  5
    8  79`.split('\n').map(s=>s.split(''));
  for (let i of range) {
    for (let j of range) {
      if (input[i][j] !== ' ') {
        solver.addClause(field[i][j][parseInt(input[i][j], 10) - 1]);
      }
    }
  }

  solver.addClause(field[0][0][4]);
  if (!solver.solve()) {
    print("No solution.");
    return;
  }

  let res = solver.extract();
  for (let i of range) {
    let row = [];
    for (let j of range) {
      for (let k of range) {
        if (res[field[i][j][k]] === 'true') {
          row.push(k + 1);
          break;
        }
      }
    }
    print(row.join(''));
  }
})(...arguments).catch(e => {
  if (e instanceof Error) {
    print(e.stack);
  } else {
    print(e);
  }
}).finally(() => {
  testRunner.notifyDone();
});

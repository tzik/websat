
let unescapedChars =
    ";,/?:@&=+$" +
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ" +
    "abcdefghijklmnopqrstuvwxyz" +
    "0123456789" +
    "-_.!~*'()#";
let unescapedCharCodes = unescapedChars.split('').map(c => c.charCodeAt());

export function decodeUTF8(buf) {
  let s = [];
  for (let c of buf) {
    if (c === 0) {
      break;
    }

    if (unescapedCharCodes.indexOf(c) !== -1) {
      s.push(String.fromCharCode(c));
    } else {
      s.push('%' + ('0' + c.toString(16)).slice(-2));
    }
  }
  return decodeURI(s.join(''));
}

export function encodeUTF8(text) {
  let b = [];
  for (let c of text) {
    if (unescapedChars.indexOf(c) !== -1) {
      b.push(c.charCodeAt());
    } else {
      let s = encodeURI(c);
      while (s.length) {
        b.push(parseInt(s.slice(1, 3), 16));
        s = s.slice(3);
      }
    }
  }
  return Uint8Array.from(b);
}

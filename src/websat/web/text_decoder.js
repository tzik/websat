
export function decodeUTF8(buf) {
  return TextDecoder.decode(buf);
}

export function encodeUTF8(text) {
  return TextEncoder.encode(text);
}

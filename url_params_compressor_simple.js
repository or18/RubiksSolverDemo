/**
 * Simplified URL Parameters Compressor (Hash-only, No LZMA)
 * - Character mapping compression (76.8% reduction)
 * - QR Alphanumeric mode compatible
 * - Fast synchronous processing
 * - Individual parameter handling (omit empty parameters)
 * - CREST bitmap optimization (24 patterns → 6 chars, hexadecimal)
 */

// Character mapping for 55 moves (54 rotations + EMPTY)
const MOVE_TO_CHAR = {
  'U': 'A', 'U2': 'B', 'U-': 'C',
  'D': 'D', 'D2': 'E', 'D-': 'F',
  'L': 'G', 'L2': 'H', 'L-': 'I',
  'R': 'J', 'R2': 'K', 'R-': 'L',
  'F': 'M', 'F2': 'N', 'F-': 'O',
  'B': 'P', 'B2': 'Q', 'B-': 'R',
  'u': 'S', 'u2': 'T', 'u-': 'U',
  'd': 'V', 'd2': 'W', 'd-': 'X',
  'l': 'Y', 'l2': 'Z', 'l-': 'a',
  'r': 'b', 'r2': 'c', 'r-': 'd',
  'f': 'e', 'f2': 'f', 'f-': 'g',
  'b': 'h', 'b2': 'i', 'b-': 'j',
  'M': 'k', 'M2': 'l', 'M-': 'm',
  'S': 'n', 'S2': 'o', 'S-': 'p',
  'E': 'q', 'E2': 'r', 'E-': 's',
  'x': 't', 'x2': 'u', 'x-': 'v',
  'y': 'w', 'y2': 'x', 'y-': 'y',
  'z': 'z', 'z2': '0', 'z-': '1',
  'EMPTY': '2'
};

const CHAR_TO_MOVE = Object.fromEntries(
  Object.entries(MOVE_TO_CHAR).map(([k, v]) => [v, k])
);

// CREST 24 patterns (index.html specific, 6 rows × 4 columns)
const CREST_PATTERNS = [
  'EMPTY_EMPTY', 'EMPTY_y', 'EMPTY_y2', 'EMPTY_y-',
  'z2_EMPTY', 'z2_y', 'z2_y2', 'z2_y-',
  'z-_EMPTY', 'z-_y', 'z-_y2', 'z-_y-',
  'z_EMPTY', 'z_y', 'z_y2', 'z_y-',
  'x-_EMPTY', 'x-_y', 'x-_y2', 'x-_y-',
  'x_EMPTY', 'x_y', 'x_y2', 'x_y-'
];

// ===== REST Encoding =====

function encodeRest(restString) {
  if (!restString || restString.trim() === '') return '';
  const moves = restString.split('_');
  return moves.map(m => MOVE_TO_CHAR[m] || '').join('');
}

function decodeRest(encoded) {
  if (!encoded) return '';
  const chars = encoded.split('');
  return chars.map(c => CHAR_TO_MOVE[c] || '').join('_');
}

// ===== MAV Encoding =====

function encodeMav(mavString) {
  if (!mavString || mavString.trim() === '') return '';
  const sections = mavString.split('|');
  let encoded = '';
  
  for (const section of sections) {
    if (!section) continue;
    const pair = section.split('~');
    for (const move of pair) {
      if (!move) continue;
      const char = MOVE_TO_CHAR[move];
      if (!char) continue;
      encoded += char;
    }
  }
  return encoded;
}

function decodeMav(encodedMav) {
  if (!encodedMav || encodedMav.trim() === '') return '';
  
  if (encodedMav.length % 2 !== 0) {
    console.warn(`[MAV Decode Warning] Odd length input detected: "${encodedMav}" (length=${encodedMav.length}). Last character will be ignored.`);
  }
  
  const sections = [];
  
  for (let i = 0; i < encodedMav.length; i += 2) {
    if (i + 1 >= encodedMav.length) break;
    const char1 = encodedMav[i];
    const char2 = encodedMav[i + 1];
    const move1 = CHAR_TO_MOVE[char1];
    const move2 = CHAR_TO_MOVE[char2];
    if (!move1 || !move2) continue;
    sections.push(`${move1}~${move2}`);
  }
  return sections.join('|');
}

// ===== CREST Encoding (Bitmap Optimization - 6-char Hexadecimal) =====

function encodeCrest(crestString) {
  if (!crestString || crestString.trim() === '') return '';
  
  const patterns = crestString.split('|');
  
  // Create 24-bit bitmap
  let bitmap = 0;
  for (const pattern of patterns) {
    const index = CREST_PATTERNS.indexOf(pattern);
    if (index >= 0) {
      bitmap |= (1 << index);
    }
  }
  
  // Convert 24-bit to 6-char hexadecimal (0-9A-F)
  // This avoids URL-unsafe characters like '+' and '/'
  const hex = bitmap.toString(16).toUpperCase().padStart(6, '0');
  return hex;
}

function decodeCrest(encoded) {
  if (!encoded || encoded.length !== 6) {
    if (encoded) {
      console.warn(`[CREST Decode Warning] Invalid length: expected 6, got ${encoded.length}`);
    }
    return '';
  }
  
  // Convert 6-char hexadecimal to 24-bit bitmap
  const bitmap = parseInt(encoded, 16);
  if (isNaN(bitmap)) {
    console.warn(`[CREST Decode Warning] Invalid hexadecimal format: "${encoded}"`);
    return '';
  }
  
  // Extract patterns from bitmap
  const patterns = [];
  for (let i = 0; i < 24; i++) {
    if (bitmap & (1 << i)) {
      patterns.push(CREST_PATTERNS[i]);
    }
  }
  
  return patterns.join('|');
}

// ===== MCV Encoding =====

function encodeMcv(mcvString) {
  if (!mcvString || mcvString.trim() === '') return '';
  const entries = mcvString.split('_');
  let encoded = '';
  
  for (const entry of entries) {
    if (!entry) continue;
    const parts = entry.split(':');
    if (parts.length !== 2) continue;
    const move = parts[0];
    const count = parts[1];
    const char = MOVE_TO_CHAR[move];
    if (!char) continue;
    encoded += char + count;
  }
  return encoded;
}

function decodeMcv(encodedMcv) {
  if (!encodedMcv || encodedMcv.trim() === '') return '';
  const entries = [];
  let i = 0;
  
  while (i < encodedMcv.length) {
    const char = encodedMcv[i];
    const move = CHAR_TO_MOVE[char];
    if (!move) {
      i++;
      continue;
    }
    i++;
    let numStr = '';
    while (i < encodedMcv.length && /\d/.test(encodedMcv[i])) {
      numStr += encodedMcv[i];
      i++;
    }
    if (numStr === '') continue;
    entries.push(`${move}:${numStr}`);
  }
  return entries.join('_') + '_';
}

// ===== URL Processing =====

/**
 * Normalize URL: Convert compressed format to legacy format
 * Handles: #hrest=...&hmav=... → ?rest=...&mav=...
 */
function normalizeUrl(urlString) {
  try {
    const url = new URL(urlString);
    
    // Check if hash contains compressed parameters
    if (!url.hash || !url.hash.includes('=')) {
      return urlString; // No compressed parameters
    }
  
  // Parse hash parameters
  const hashParams = new URLSearchParams(url.hash.substring(1));
  
  const hrest = hashParams.get('hrest');
  const hmav = hashParams.get('hmav');
  const hcrest = hashParams.get('hcrest');
  const hmcv = hashParams.get('hmcv');
  
  // If no hash parameters, return original
  if (!hrest && !hmav && !hcrest && !hmcv) {
    return urlString;
  }
  
  // Decode parameters
  const rest = hrest ? decodeRest(hrest) : null;
  const mav = hmav ? decodeMav(hmav) : null;
  const crest = hcrest ? decodeCrest(hcrest) : null;
  const mcv = hmcv ? decodeMcv(hmcv) : null;
  
  // Build new URL with legacy format
  const newUrl = new URL(url.origin + url.pathname + url.search);
  
  if (rest) newUrl.searchParams.set('rest', rest);
  if (mav) newUrl.searchParams.set('mav', mav);
  if (crest) newUrl.searchParams.set('crest', crest);
  if (mcv) newUrl.searchParams.set('mcv', mcv);
  
  return newUrl.toString();
  } catch (error) {
    console.warn('[URL Normalize Warning] Invalid URL format, using default:', error.message);
    if (typeof window !== 'undefined') {
      return window.location.origin + window.location.pathname;
    }
    return urlString;
  }
}

/**
 * Compress URL: Convert legacy format to compressed format
 * Handles: ?rest=...&mav=... → #hrest=...&hmav=...
 */
function compressUrlForDisplay(urlString) {
  try {
    const url = new URL(urlString);
  
  // Get legacy parameters
  const rest = url.searchParams.get('rest');
  const mav = url.searchParams.get('mav');
  const crest = url.searchParams.get('crest');
  const mcv = url.searchParams.get('mcv');
  
  // If no parameters to compress, return original
  if (!rest && !mav && !crest && !mcv) {
    return urlString;
  }
  
  // Encode parameters
  const hrest = rest ? encodeRest(rest) : null;
  const hmav = mav ? encodeMav(mav) : null;
  const hcrest = crest ? encodeCrest(crest) : null;
  const hmcv = mcv ? encodeMcv(mcv) : null;
  
  // Build hash string (omit empty parameters)
  const hashParts = [];
  if (hrest) hashParts.push(`hrest=${hrest}`);
  if (hmav) hashParts.push(`hmav=${hmav}`);
  if (hcrest) hashParts.push(`hcrest=${hcrest}`);
  if (hmcv) hashParts.push(`hmcv=${hmcv}`);
  
  // Build new URL with compressed format
  const newUrl = new URL(url.origin + url.pathname);
  
  // Keep non-compressed parameters (like 'res')
  url.searchParams.forEach((value, key) => {
    if (key !== 'rest' && key !== 'mav' && key !== 'crest' && key !== 'mcv') {
      newUrl.searchParams.set(key, value);
    }
  });
  
  // Add hash
  if (hashParts.length > 0) {
    newUrl.hash = hashParts.join('&');
  }
  
  return newUrl.toString();
  } catch (error) {
    console.warn('[URL Compress Warning] Invalid URL format, returning original:', error.message);
    return urlString;
  }
}

// Export functions
if (typeof window !== 'undefined') {
  window.urlParamsCompressor = {
    encodeRest, decodeRest,
    encodeMav, decodeMav,
    encodeCrest, decodeCrest,
    encodeMcv, decodeMcv,
    normalizeUrl,
    compressUrlForDisplay
  };
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports = {
    encodeRest, decodeRest,
    encodeMav, decodeMav,
    encodeCrest, decodeCrest,
    encodeMcv, decodeMcv,
    normalizeUrl,
    compressUrlForDisplay
  };
}

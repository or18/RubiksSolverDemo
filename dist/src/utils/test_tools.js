const t = require('./tools.js');

function equal(a,b){
  if (a!==b) throw new Error(`Expected ${JSON.stringify(b)}, got ${JSON.stringify(a)}`);
}

function ok(v,msg){ if (!v) throw new Error('Assertion failed: '+(msg||String(v))); }

(async ()=>{
  const failures = [];
  const results = [];
  const run = async (name, fn) => {
    try {
      await fn();
      results.push({name, ok:true});
      console.log('\u2714', name);
    } catch (e) {
      results.push({name, ok:false, error:e});
      console.error('\u2716', name, e && e.message);
      failures.push({name, error:e});
    }
  };

  // Synchronous JS-only helpers
  await run('buildRestFromArray', ()=>{
    const out = t.buildRestFromArray(['R','R2',"R'"]);
    equal(out, 'R_R2_R-');
  });

  await run('normalizeRestForCpp', ()=>{
    const out = t.normalizeRestForCpp("R_R'_");
    equal(out, 'R_R-');
  });

  await run('buildMcvFromObject', ()=>{
    const out = t.buildMcvFromObject(['R','R2',"R'"], { R:1, "R'":2 });
    equal(out, 'R:1_R-:2');
  });

  await run('buildMavFromPairs', ()=>{
    const out = t.buildMavFromPairs(['R','R2',"R'"], [['R','R2'], ["R'","R"], ['', 'R2']]);
    equal(out, 'R~R2|R-~R|EMPTY~R2');
  });

  await run('validateRest/validateMav/validateMcv/validateCrest', ()=>{
    const rest = 'R_R2_R-';
    ok(t.validateRest(rest) === true);
    const mav = 'R~R2|EMPTY~R2';
    ok(t.validateMav(rest, mav) === true);
    const mcv = 'R:1_R-:2';
    ok(t.validateMcv(rest, mcv) === true);
    ok(t.validateCrest('x_y') === true);
  });

  await run('scr_fix and scrambleFilter', ()=>{
    const s = "R U R' F2";
    const fixed = t.scr_fix(s);
    ok(typeof fixed === 'string');
    const sf = t.scrambleFilter(s);
    ok(typeof sf === 'string');
  });

  // Async helpers that may depend on C++ module — run but accept skips
  await run('buildCrestFromArray (array-of-pairs only)', async ()=>{
    const out = await t.buildCrestFromArray([ ['x','y'], ['z2','y'] ]);
    // order preserved and deduped by implementation
    equal(out, 'x_y|z2_y');
  });

  await run('getCenterOffset (may be skipped if C++ module missing)', async ()=>{
    try {
      const c = await t.getCenterOffset('x2');
      // if returns, it should be a string
      ok(typeof c === 'string');
      console.log('getCenterOffset ->', c);
    } catch (e) {
      // Mark as skipped rather than failing when C++ helper missing
      if (/not available in loaded module|function not available/.test(e && e.message)) {
        console.warn('skipped getCenterOffset: native module unavailable');
        return;
      }
      throw e;
    }
  });

  await run('invertAlg (may be skipped if C++ module missing)', async ()=>{
    try {
      const r = await t.invertAlg("R U R'");
      ok(typeof r === 'string');
      console.log('invertAlg ->', r.split('\n')[0]);
    } catch (e) {
      if (/not available in loaded module|function not available/.test(e && e.message)) {
        console.warn('skipped invertAlg: native module unavailable');
        return;
      }
      throw e;
    }
  });

  await run('mirrorAlg (may be skipped if C++ module missing)', async ()=>{
    try {
      const m = await t.mirrorAlg("R U R'");
      ok(typeof m === 'string');
      console.log('mirrorAlg ->', m.split('\n')[0]);
    } catch (e) {
      if (/not available in loaded module|function not available/.test(e && e.message)) {
        console.warn('skipped mirrorAlg: native module unavailable');
        return;
      }
      throw e;
    }
  });

  await run('splitRotationAlg (may be skipped if C++ module missing)', async ()=>{
    try {
      const parts = await t.splitRotationAlg("R U R' x2");
      ok(parts && typeof parts.rotation === 'string' && typeof parts.scramble === 'string');
      console.log('splitRotationAlg ->', parts);
    } catch (e) {
      if (/not available in loaded module|function not available/.test(e && e.message)) {
        console.warn('skipped splitRotationAlg: native module unavailable');
        return;
      }
      throw e;
    }
  });

  await run('rotateAlg (may be skipped if C++ module missing)', async ()=>{
    try {
      const out = await t.rotateAlg("R U R'","x2");
      ok(typeof out === 'string');
      console.log('rotateAlg ->', out.split('\n')[0]);
    } catch (e) {
      if (/not available in loaded module|function not available/.test(e && e.message)) {
        console.warn('skipped rotateAlg: native module unavailable');
        return;
      }
      throw e;
    }
  });

  await run('generateTwoPhaseInput (may be skipped if C++ module missing)', async ()=>{
    try {
      const s54 = await t.generateTwoPhaseInput("R U R'");
      ok(typeof s54 === 'string');
      // If meaningful, should be 54 chars for 3x3
      if (s54) console.log('generateTwoPhaseInput length ->', s54.length);
    } catch (e) {
      if (/not available in loaded module|function not available/.test(e && e.message)) {
        console.warn('skipped generateTwoPhaseInput: native module unavailable');
        return;
      }
      throw e;
    }
  });

  // Report
  if (failures.length === 0) {
    console.log('\nALL TESTS PASSED');
    process.exit(0);
  } else {
    console.error('\nTESTS FAILED:', failures.length);
    failures.forEach(f=> console.error('*', f.name, '-', f.error && f.error.message));
    process.exit(2);
  }
})();

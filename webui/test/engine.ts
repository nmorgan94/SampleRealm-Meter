import { readJson } from '@samplerealm/modules/demo/engine.ts';
import { engineAt } from '@samplerealm/modules/testing/engine.ts';
import { localEnginePath } from '../localEngine.ts';
import { paramLookup } from '@samplerealm/modules/backend.ts';
import type { MeterInfo } from '../src/backend.ts';

// Makes a fresh engine from the last wasm build
export const engine = engineAt(localEnginePath());

const first = engine();

export const { info } = first;

export const meter = readJson<MeterInfo>(first.engine, first.engine.meterInfo());

export const param = paramLookup(new Map(info.params.map((p) => [p.id, p])));

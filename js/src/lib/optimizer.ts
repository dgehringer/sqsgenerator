import MainModule from '$lib/core.mjs';
import type { ValidationError } from 'svelte-jsoneditor';

export interface RunConfig {
    prec: 'single' | 'double' | 0 | 1;
    sublattice_mode: 'interact' | 'split';
    iteration_mode: 'random' | 'systematic';
}

export type ParseError = {
    key: string;
    msg: string;
    code: number;
    parameter: string | null;
};

export type Left<E> = {
    ok: false;
    value: E;
};

export type Right<T> = { ok: true; value: T };
export type Result<T, E> = Left<E> | Right<T>;

export interface SqsStatistics {
    working: number;
    finished: number;
    best_rank: number;
    best_objective: number;
}

export class SqsgenOptimizer {
    private module: MainModule;

    constructor(module: MainModule) {
        this.module = module;
    }

    async optimizeAsync(
        config: any,
        precision: 0 | 1,
        callback: (stats: SqsStatistics) => boolean | undefined
    ): Promise<any> {
        const prec = precision === 0 ? this.module.Prec.single : this.module.Prec.double;
        return await this.module.optimizeAsync(config, prec, callback);
    }

    private isError(result: any): boolean {
        return 'key' in result && 'msg' in result && 'code' in result;
    }

    private fixupPath(flattenedKey?: string): string[] {
        if (!flattenedKey) {
            return [];
        }
        const keys = new Map<string, string[]>([['sites', ['composition', 'sites']]]);
        return keys.get(flattenedKey) ?? [flattenedKey];
    }

    validate(config: any) {
        const parsed = this.parseConfig(config);
        return parsed.ok
            ? []
            : [
                    {
                        path: this.fixupPath(parsed.value.key),
                        message: parsed.value.msg,
                        severity: 'warning'
                    } as ValidationError
                ];
    }

    parseConfig(config: any): Result<RunConfig, ParseError> {
        try {
            const result = this.module.parseConfig(config);
            return { ok: !this.isError(result), value: result };
        } catch (e: any) {
            return {
                ok: false,
                value: {
                    key: 'unknown',
                    code: -1,
                    msg: 'An unknown error occurred while parsing the configuration.'
                } as ParseError
            };
        }
    }
}

export async function loadOptimizer() {
    const module = await import('$lib/core.mjs');
    return new SqsgenOptimizer(await module.default());
}

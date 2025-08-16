import {sveltekit} from '@sveltejs/kit/vite';
import {defineConfig} from 'vite';
import wasm from 'vite-plugin-wasm';

const crossOriginHeaders = {
    name: 'cross-origin-headers',
    configureServer(server) {
        server.middlewares.use((_req, res, next) => {
            res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
            res.setHeader('Cross-Origin-Embedder-Policy', 'require-corp');
            next();
        });
    }
};
export default defineConfig({
    plugins: [wasm(), sveltekit(), crossOriginHeaders],
    build: {
        target: 'esnext' ,
        rollupOptions: {
            output: {
                manualChunks: undefined, // Disable code-splitting
                inlineDynamicImports: true, // Inline dynamic imports
                format: 'es', // ES modules format (required for top-level await)
            },
        },
    },
    worker: {
        format: 'es' // This is the key change needed
    },
    test: {
        projects: [
            {
                extends: './vite.config.ts',
                test: {
                    name: 'server',
                    environment: 'node',
                    include: ['src/**/*.{test,spec}.{js,ts}'],
                    exclude: ['src/**/*.svelte.{test,spec}.{js,ts}']
                }
            }
        ]
    }
});

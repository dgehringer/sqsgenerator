import { sveltekit } from '@sveltejs/kit/vite';
import { defineConfig } from 'vite';

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
    plugins: [crossOriginHeaders, sveltekit()],
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

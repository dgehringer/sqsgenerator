export function downloadAsFile(content: string | Uint8Array, filename: string, mimeType?: string): void {
    const blob = new Blob([content], {type: mimeType ?? 'text/plain'});
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
}

export async function compressData(data: any) {
    const jsonString = JSON.stringify(data);
    const encoder = new TextEncoder();
    const encoded = encoder.encode(jsonString);

    // Compression stream (modern browsers)
    const cs = new CompressionStream('gzip');
    const writer = cs.writable.getWriter();
    writer.write(encoded);
    writer.close();

    const compressed = await new Response(cs.readable).arrayBuffer();
    return btoa(String.fromCharCode(...new Uint8Array(compressed)));
}


export async function decompressData(compressedBase64: string) {

    const compressedData = Uint8Array.from(atob(compressedBase64), c => c.charCodeAt(0));

    const ds = new DecompressionStream('gzip');
    const writer = ds.writable.getWriter();
    writer.write(compressedData);
    writer.close();

    const decompressed = await new Response(ds.readable).arrayBuffer();
    const decoder = new TextDecoder();
    return JSON.parse(decoder.decode(decompressed));

}

export function defaultConfig() {
    return {
        iterations: 1000000,
        structure: {
            species: ['Fe', 'Fe'],
            supercell: [3, 3, 3],
            lattice: [
                [2.86, 0.0, 0.0],
                [0.0, 2.86, 0.0],
                [0.0, 0.0, 2.86]
            ],
            coords: [
                [0.0, 0.0, 0.0],
                [0.5, 0.5, 0.5]
            ]
        },
        composition: [
            {
                Fe: 45,
                Al: 9
            }
        ],
        target_objective: 0
    };
}

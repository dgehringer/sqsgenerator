export function downloadAsFile(content: string | Uint8Array, filename: string, mimeType?: string): void {
    const blob = new Blob([content], { type: mimeType ?? 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
}

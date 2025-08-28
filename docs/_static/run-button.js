async function compressData(data) {
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

function cleanNumberedLines(input) {
  input.split('\n').forEach((line) => console.log(line.replace(/^\d+/, '')));
  return input.split('\n').map(line => line.replace(/^\s*\d+/, '')).join('\n');
}


function attachRunButtons() {
  // Find all JSON code blocks
  document.querySelectorAll('div.highlight-json.runbutton').forEach((block) => {
    // Skip if button already added
    if (block.querySelector('.run-webapp-button')) return;
    block.style.position = 'relative';
    // Create button
    const btn = document.createElement('button');
    btn.textContent = '▶';  // play symbol
    btn.title = 'Run code in Web App';
    btn.className
        = 'run-in-webapp-btn sd-sphinx-override sd-btn sd-text-wrap sd-btn-outline-primary reference external';
    // Style button as top-right overlay

    // Button click handler
    btn.addEventListener('click', () => {
      const code = cleanNumberedLines(block.querySelector('.highlight').innerText.trim());
      try {
        JSON.parse(code);
      } catch (e) {
        console.error(code);
        console.error(e);
        alert('Invalid JSON in code block!' + e);
        return;
      }

      compressData(code).then((compressedBase64) => {
        const url = `https://sqsgen.gehringer.tech/?config=${encodeURIComponent(compressedBase64)}`;
        window.open(url, '_blank');
      })
    });

    // Insert button after the code block
    block.appendChild(btn);
  });
}

// Run after page load
document.addEventListener('DOMContentLoaded', attachRunButtons);

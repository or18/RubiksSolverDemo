(function () {
    const metaDesc = document.querySelector('meta[name="description"]');
    const description = metaDesc ? metaDesc.getAttribute('content') : document.title;
    const title = document.title || "Rubik's Cube Solver & Trainer";
    const currentUrl = window.location.href.split('?')[0].split('#')[0];
    const path = window.location.pathname;

    let schemaData;

    if (path.endsWith('documentation.html')) {
        schemaData = {
            "@context": "https://schema.org",
            "@type": "WebPage",
            "name": title,
            "url": currentUrl,
            "description": description,
            "inLanguage": "en"
        };
    } 
    else {
        schemaData = {
            "@context": "https://schema.org",
            "@type": "WebApplication",
            "name": title,
            "url": currentUrl,
            "description": description,
            "applicationCategory": "UtilitiesApplication",
            "keywords": "Rubik's Cube, Speedcubing, F2L, XCross, EOCross, CFOP, ZZ, Scrambler, Solver, Trainer",
            "operatingSystem": "All",
            "browserRequirements": "Requires WebAssembly and JavaScript support",
            "offers": {
                "@type": "Offer",
                "price": "0",
                "priceCurrency": "USD"
            }
        };
    }

    const script = document.createElement('script');
    script.type = 'application/ld+json';
    script.textContent = JSON.stringify(schemaData, null, 2);
    document.head.appendChild(script);
})();

import os
import re

BASE_URL = "https://or18.github.io/RubiksSolverDemo"

for filename in os.listdir("."):
    if not filename.endswith(".html"):
        continue

    with open(filename, "r", encoding="utf-8") as f:
        html = f.read()

    title_match = re.search(r"<title>(.*?)</title>", html, re.IGNORECASE)
    desc_match = re.search(r'<meta\s+name=["\']description["\']\s+content=["\'](.*?)["\']', html, re.IGNORECASE)

    title = title_match.group(1).strip() if title_match else "Rubik's Cube Solver & Trainer"
    description = desc_match.group(1).strip() if desc_match else title

    og_type = "article" if filename == "documentation.html" else "website"
    page_url = f"{BASE_URL}/{filename}"

    ogp_tags = f"""<!-- OGP Auto-Injected -->
    <meta property="og:title" content="{title}">
    <meta property="og:description" content="{description}">
    <meta property="og:url" content="{page_url}">
    <meta property="og:type" content="{og_type}">
    <meta name="twitter:card" content="summary">
    <!-- /OGP Auto-Injected -->"""

    if "<!-- OGP Auto-Injected -->" in html:
        html = re.sub(r"<!-- OGP Auto-Injected -->.*?<!-- /OGP Auto-Injected -->", ogp_tags, html, flags=re.DOTALL)
    else:
        html = re.sub(r"(<head[^>]*>)", r"\1\n    " + ogp_tags, html, count=1, flags=re.IGNORECASE)

    with open(filename, "w", encoding="utf-8") as f:
        f.write(html)

    print(f"Updated: {filename} ({og_type})")

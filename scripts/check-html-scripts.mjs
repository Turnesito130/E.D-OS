import { readdirSync, readFileSync } from "node:fs";
import { join, relative } from "node:path";
import { Script } from "node:vm";

function walk(directory) {
  return readdirSync(directory, { withFileTypes: true }).flatMap(entry => {
    const path = join(directory, entry.name);
    return entry.isDirectory() ? walk(path) : (entry.isFile() && path.endsWith(".html") ? [path] : []);
  });
}

const htmlFiles = ["index.html", "connect.html", ...walk("apps")];
let checked = 0;
for (const file of htmlFiles) {
  const html = readFileSync(file, "utf8");
  const pattern = /<script\b([^>]*)>([\s\S]*?)<\/script>/gi;
  let match;
  let index = 0;
  while ((match = pattern.exec(html))) {
    const attributes = match[1] || "";
    if (/\bsrc\s*=/.test(attributes) || /type\s*=\s*["']application\/ld\+json["']/i.test(attributes)) continue;
    new Script(match[2], { filename: relative(process.cwd(), file) + ":inline-" + (++index) });
    checked += 1;
  }
}
process.stdout.write("HTML_INLINE_SCRIPTS_OK " + checked + "\n");

import { createServer } from "node:http";

// Minimal path-style S3 HTTP fixture for deterministic integration tests.
// It exercises the AWS SDK's presigned HTTP flow, not a production S3 provider.
export function createS3ProtocolFixture() {
  const objects = new Map();
  const server = createServer(async (req, res) => {
    const pathname = decodeURIComponent(new URL(req.url, "http://localhost").pathname);
    const parts = pathname.split("/").filter(Boolean);
    if (parts.length < 1) {
      res.writeHead(404).end();
      return;
    }
    const key = parts.slice(1).join("/");
    if (req.method === "PUT" && !key) {
      res.writeHead(200).end();
      return;
    }
    if (!key) {
      res.writeHead(404).end();
      return;
    }
    if (req.method === "PUT") {
      const chunks = [];
      for await (const chunk of req) chunks.push(chunk);
      const body = Buffer.concat(chunks);
      objects.set(key, { body, contentType: req.headers["content-type"] || "application/octet-stream" });
      res.writeHead(200, { etag: '"mock-etag"' }).end();
      return;
    }
    const object = objects.get(key);
    if (!object) {
      res.writeHead(404, { "content-type": "application/xml" })
        .end("<Error><Code>NoSuchKey</Code><Message>Not found</Message></Error>");
      return;
    }
    if (req.method === "HEAD") {
      res.writeHead(200, {
        "content-length": String(object.body.length),
        "content-type": object.contentType,
        etag: '"mock-etag"'
      }).end();
      return;
    }
    if (req.method === "GET") {
      res.writeHead(200, {
        "content-length": String(object.body.length),
        "content-type": object.contentType
      }).end(object.body);
      return;
    }
    if (req.method === "DELETE") {
      objects.delete(key);
      res.writeHead(204).end();
      return;
    }
    res.writeHead(405).end();
  });
  return { server, objects };
}

// Removes abandoned uploads without holding database locks during S3 calls.
// Failed object deletions remain in "deleting" and are retried on the next pass.
export async function cleanupAbandonedUploads({
  pool, s3, bucket, olderThanMinutes = 60, batchSize = 100
}) {
  if (!pool || !s3 || !bucket) return { deleted: 0, failed: 0 };
  const client = await pool.connect();
  let candidates;
  try {
    await client.query("BEGIN");
    const claimed = await client.query(
      `WITH candidates AS (
         SELECT id FROM cloud_files
          WHERE status IN ('uploading', 'deleting')
            AND created_at < now() - ($1::int * interval '1 minute')
          ORDER BY created_at
          LIMIT $2
          FOR UPDATE SKIP LOCKED
       )
       UPDATE cloud_files AS files
          SET status = 'deleting'
         FROM candidates
        WHERE files.id = candidates.id
       RETURNING files.id, files.object_key`,
      [olderThanMinutes, batchSize]
    );
    candidates = claimed.rows;
    await client.query("COMMIT");
  } catch (error) {
    await client.query("ROLLBACK");
    throw error;
  } finally {
    client.release();
  }

  let deleted = 0;
  let failed = 0;
  for (const file of candidates) {
    try {
      await s3.send(new DeleteObjectCommand({ Bucket: bucket, Key: file.object_key }));
      await pool.query("DELETE FROM cloud_files WHERE id = $1 AND status = 'deleting'", [file.id]);
      deleted++;
    } catch (error) {
      failed++;
    }
  }
  return { deleted, failed, examined: candidates.length };
}

// Keep the S3 command import local to this maintenance module.
import { DeleteObjectCommand } from "@aws-sdk/client-s3";

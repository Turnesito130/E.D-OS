# E.D. Ecosystem API — prototipo 0.1

Este servicio es el primer backend ejecutable de E.D. ID y del inventario de dispositivos. Está separado del escritorio web y requiere un servidor Node.js y PostgreSQL.

## Incluye

- Registro de cuenta con correo y contraseña.
- Hash de contraseña con scrypt y sal aleatoria.
- Inicio de sesión con token aleatorio opaco; en PostgreSQL solo se guarda su SHA-256.
- Sesiones con caducidad de 12 horas y cierre de sesión revocable.
- Consulta de la cuenta autenticada.
- Alta del primer dispositivo de la cuenta, listado y revocación de dispositivos. Los siguientes requieren emparejamiento.
- Consultas SQL parametrizadas y límite básico de intentos por IP en memoria.

## No incluye todavía

- Verificación de correo, recuperación de contraseña ni passkeys.
- E.D. Cloud experimental: metadatos en PostgreSQL y bytes en almacenamiento S3-compatible mediante URLs firmadas. Requiere configurar el proveedor; no está desplegado ni auditado.
- Rate limiting distribuido, MFA, auditoría completa ni recuperación segura de claves.
- Despliegue, dominio, certificados TLS, copias de seguridad o monitorización.

**No usar con cuentas ni datos personales reales.** Es una base de desarrollo, no un servicio de producción auditado.

## Requisitos

- Node.js 20 o superior.
- PostgreSQL accesible desde el servidor.
- HTTPS terminado en un proxy inverso o plataforma de despliegue correctamente configurada.

## Preparación local

1. Crear una base de datos PostgreSQL vacía.
2. Ejecutar el esquema:

```sh
psql "$DATABASE_URL" -f schema.sql
```

3. Instalar dependencias:

```sh
npm install
```

4. Configurar las variables de entorno:

- `DATABASE_URL`: cadena de conexión PostgreSQL.
- `PORT`: opcional; por defecto 8787.
- `PGSSLMODE=require`: activa TLS para PostgreSQL cuando el proveedor lo requiera.
- `CORS_ORIGINS`: lista separada por comas de orígenes web autorizados, por ejemplo `https://turnesito130.github.io`. No uses `*`.\n- `S3_ENDPOINT`, `S3_BUCKET`, `S3_ACCESS_KEY_ID`, `S3_SECRET_ACCESS_KEY` y opcionalmente `S3_REGION`: configuración S3-compatible; secretos solo en el gestor de secretos del servidor.

5. Configurar `CORS_ORIGINS` con el origen exacto desde el que se abrirá `apps/ecosystem.html`.
6. Iniciar:

```sh
npm run check
npm start
```

La ruta `GET /health` solo comprueba que el proceso responde; no certifica que toda la infraestructura esté lista.

## API 0.1

Todas las respuestas son JSON. Las rutas privadas requieren `Authorization: Bearer <token>`.

| Método | Ruta | Acceso | Función |
|---|---|---|---|
| GET | `/health` | Público | Estado básico del proceso |
| POST | `/v1/auth/register` | Público | Crear cuenta |
| POST | `/v1/auth/login` | Público | Iniciar sesión |
| POST | `/v1/auth/logout` | Sesión | Revocar sesión actual |
| GET | `/v1/account/me` | Sesión | Consultar identidad |
| GET | `/v1/devices` | Sesión | Listar dispositivos activos |
| POST | `/v1/devices` | Sesión | Registrar dispositivo |
| POST | `/v1/devices/{id}/revoke` | Sesión | Revocar un dispositivo propio |
| POST | `/v1/pairing/requests` | Público | Crear solicitud temporal en el nuevo dispositivo |
| POST | `/v1/pairing/approve` | Sesión | Aprobar solicitud desde una sesión de la cuenta |
| POST | `/v1/pairing/claim` | Secreto temporal | Completar una vez el emparejamiento y obtener sesión |
| GET | `/v1/sync/items` | Sesión | Listar hasta 500 registros JSON |
| PUT | `/v1/sync/items/{key}` | Sesión | Crear/actualizar con control de versión |
| DELETE | `/v1/sync/items/{key}` | Sesión | Borrado lógico con control de versión |\n| GET | `/v1/cloud/files` | Sesión | Listar archivos completados |\n| POST | `/v1/cloud/files` | Sesión | Crear reserva y URL firmada de subida (máx. 50 MB) |\n| POST | `/v1/cloud/files/{id}/complete` | Sesión | Verificar archivo tras la subida |\n| GET | `/v1/cloud/files/{id}/download` | Sesión | Obtener URL temporal de descarga |\n| DELETE | `/v1/cloud/files/{id}` | Sesión | Eliminar objeto y metadatos |

Ejemplo de registro:

```json
{
  "email": "persona@example.com",
  "password": "una-frase-larga-de-12-caracteres"
}
```

Ejemplo de alta de dispositivo:

```json
{
  "deviceName": "Mi móvil E.D.",
  "deviceType": "phone",
  "platform": "android"
}
```

Tipos admitidos: `phone`, `tablet`, `computer`, `watch`, `earbuds`, `other`.

## E.D. Cloud experimental\n\nEl cliente solicita una URL PUT firmada y sube directamente al almacenamiento de objetos. Después llama a `POST /v1/cloud/files/{id}/complete`; el servidor comprueba tamaño y tipo MIME con HEAD antes de marcar el archivo como disponible. Las descargas usan URLs GET firmadas de cinco minutos. Límite: 50 MB por archivo y cuota lógica de 2 GB por cuenta. Configura CORS del bucket para permitir PUT desde el origen del escritorio y el header `Content-Type`. Las reservas de cuota se serializan por cuenta con bloqueos transaccionales de PostgreSQL. Las cargas que siguen en estado `uploading` tras 60 minutos pasan a `deleting`; un proceso periódico las elimina del almacenamiento y después de la base de datos. Si el proveedor falla, la fila permanece en `deleting` para reintentarse en la siguiente ejecución. El barrido se inicia 30 segundos después del arranque y se repite cada 15 minutos. No hay cifrado E2EE ni análisis antimalware.\n\nSi faltan variables S3, las rutas Cloud devuelven 503; no se guarda el archivo localmente como sustituto.\n\n## Reglas antes de producción

- Añadir verificación de correo y recuperación de cuenta segura.
- Añadir una pantalla cliente que cree y apruebe emparejamientos, y pruebas automatizadas de caducidad, uso único y revocación.
- Incorporar límites de intentos distribuidos y protección frente a abuso.
- Configurar HTTPS, secretos fuera del repositorio, copias de seguridad cifradas y monitorización.
- Añadir pruebas automatizadas de aislamiento entre cuentas, caducidad, conflictos de sincronización y revocación.
- Realizar revisión de seguridad independiente.
- Verificar CORS del bucket desde un navegador real; la prueba MinIO de CI cubre PUT/HEAD/GET/DELETE desde Node, no la política CORS del navegador.

## Flujo de emparejamiento

1. El nuevo equipo envía `POST /v1/pairing/requests` con `deviceName`, `deviceType` y `platform`.
2. La respuesta contiene `requestId`, `pairingCode` (8 cifras), `pollSecret` (secreto privado) y caducidad de cinco minutos. Mostrar el código en el nuevo equipo; guardar el secreto solo en memoria segura del cliente.
3. En una sesión autenticada de la cuenta, el usuario selecciona un dispositivo ya vinculado y envía `POST /v1/pairing/approve` con `requestId`, `pairingCode` y `approvedByDeviceId`.
4. El nuevo equipo envía `POST /v1/pairing/claim` con `requestId` y `pollSecret`. Mientras espera, recibe HTTP 202. Tras aprobarse, recibe una sesión una sola vez.

Ejemplo de creación:

```json
{
  "deviceName": "Móvil E.D.",
  "deviceType": "phone",
  "platform": "android"
}
```

Ejemplo de aprobación:

```json
{
  "requestId": "UUID-devuelto-al-crear",
  "pairingCode": "12345678",
  "approvedByDeviceId": "UUID-de-un-dispositivo-de-la-cuenta"
}
```

El código de ocho cifras caduca en cinco minutos y solo se guarda su hash. El secreto de consulta es independiente del código. La aprobación exige una sesión válida de la cuenta y un ID de dispositivo activo de esa misma cuenta. El backend no implementa todavía escaneo QR ni límites de intentos distribuidos. La interfaz experimental está en `apps/ecosystem.html`; necesita una URL HTTPS de API desplegada y no conserva el token de sesión al cerrar la página. Por tanto, esta es una API experimental y no una garantía de seguridad de producción.

## Sincronización JSON experimental

- `PUT /v1/sync/items/{key}` requiere `expectedVersion: 0` al crear y la versión devuelta para actualizar.
- Si otro dispositivo ya modificó el elemento, responde HTTP 409; el cliente debe descargar el estado actual y resolver el conflicto explícitamente.
- `DELETE` requiere la versión actual y crea un tombstone versionado.
- `GET` devuelve como máximo 500 elementos recientes.
- No se sincronizan archivos, fotos ni secretos; el contenido se almacena como JSONB en el servidor y **no tiene cifrado de extremo a extremo**.

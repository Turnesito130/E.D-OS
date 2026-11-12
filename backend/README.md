# E.D. Ecosystem API — prototipo 0.1

Este servicio es el primer backend ejecutable de E.D. ID y del inventario de dispositivos. Está separado del escritorio web y requiere un servidor Node.js y PostgreSQL.

## Incluye

- Registro de cuenta con correo y contraseña.
- Hash de contraseña con scrypt y sal aleatoria.
- Inicio de sesión con token aleatorio opaco; en PostgreSQL solo se guarda su SHA-256.
- Sesiones con caducidad de 12 horas y cierre de sesión revocable.
- Consulta de la cuenta autenticada.
- Alta, listado y revocación de dispositivos asociados a la cuenta.
- Consultas SQL parametrizadas y límite básico de intentos por IP en memoria.

## No incluye todavía

- Verificación de correo, recuperación de contraseña ni passkeys.
- 
- Sincronización de archivos, E.D. Cloud, TURN ni señalización WebRTC. La API solo sincroniza registros JSON pequeños.
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

5. Iniciar:

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
| POST | `/v1/devices/{id}/revoke` | Sesión | Revocar un dispositivo propio |\n| POST | `/v1/pairing/requests` | Público | Crear solicitud temporal en el nuevo dispositivo |\n| POST | `/v1/pairing/approve` | Sesión | Aprobar solicitud desde un dispositivo registrado |\n| POST | `/v1/pairing/claim` | Secreto temporal | Completar una vez el emparejamiento y obtener sesión |\n| GET | `/v1/sync/items` | Sesión | Listar registros JSON sincronizados |\n| PUT | `/v1/sync/items/{key}` | Sesión | Crear/actualizar con control de versión |\n| DELETE | `/v1/sync/items/{key}` | Sesión | Borrado lógico con control de versión |

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

## Reglas antes de producción

- Añadir verificación de correo y recuperación de cuenta segura.
- Añadir una pantalla cliente que cree y apruebe emparejamientos, y pruebas automatizadas de caducidad, uso único y revocación.
- Incorporar límites de intentos distribuidos y protección frente a abuso.
- Configurar HTTPS, secretos fuera del repositorio, copias de seguridad cifradas y monitorización.
- Añadir pruebas automatizadas de aislamiento entre cuentas, caducidad, conflictos de sincronización y revocación.
- Realizar revisión de seguridad independiente.

# E.D-OS — Cuentas, dispositivos y seguridad (diseño inicial)

**Estado:** hay un backend experimental inicial de cuentas, sesiones e inventario de dispositivos en `backend/`. No está desplegado ni probado en un servidor real; no hay todavía emparejamiento confirmado ni sincronización.

## 1. Principios

- Una cuenta representa a una persona, no a un dispositivo.
- Cada dispositivo debe registrarse y aprobarse explícitamente.
- Una sesión no concede acceso automático a todos los dispositivos o archivos.
- Los permisos se conceden por finalidad: mensajes, archivos, notificaciones, ubicación u otras capacidades.
- El usuario puede consultar y revocar dispositivos y sesiones.
- La sincronización debe ser opcional y mostrar qué datos salen del dispositivo.
- Los proyectos E.D. personal, E.D-OS e IA comercial mantienen identidades y almacenes separados.

## 2. Entidades conceptuales

| Entidad | Función | Datos mínimos |
|---|---|---|
| Cuenta | Identidad del usuario | identificador interno, credenciales protegidas, estado |
| Dispositivo | Equipo autorizado | identificador aleatorio, nombre elegido, plataforma, última actividad |
| Sesión | Acceso temporal | token de sesión, caducidad, dispositivo asociado |
| Permiso | Autorización limitada | recurso, acción, alcance, fecha de concesión |
| Elemento sincronizado | Dato elegido para sincronizar | identificador, versión, propietario, metadatos mínimos |

No se deben guardar contraseñas en texto plano. Los tokens no deben incluirse en URLs, registros públicos ni repositorios.

## 3. Emparejamiento de dispositivos

1. El usuario inicia el emparejamiento desde un dispositivo ya autenticado.
2. El nuevo dispositivo presenta un código de un solo uso y con caducidad.
3. El usuario confirma el nombre y tipo de dispositivo en ambos extremos.
4. El servicio vincula el dispositivo a la cuenta y emite credenciales revocables.
5. El usuario puede revocar el vínculo desde la lista de dispositivos.

El backend 0.1 permite registrar un dispositivo directamente desde una sesión ya autenticada, pero todavía no implementa este flujo de emparejamiento. El prototipo WebRTC tampoco prueba por sí mismo la identidad del otro extremo.

## 4. Sincronización

- Mantener una copia local cuando sea posible.
- Sincronizar solo carpetas o categorías elegidas por el usuario.
- Cifrar el transporte y proteger los datos almacenados en servidor.
- Diseñar resolución de conflictos con versiones; nunca sobrescribir silenciosamente cambios.
- Permitir pausar, consultar el estado y borrar datos sincronizados.
- Mostrar claramente si un archivo está solo en el dispositivo, sincronizado o compartido.

## 5. Amenazas que deben contemplarse

- Robo o pérdida de un dispositivo.
- Robo de sesión o credenciales.
- Emparejamiento con un dispositivo equivocado.
- Acceso de una aplicación a datos de otra.
- Servidor comprometido o indisponible.
- Conflictos y pérdida de datos durante sincronización.
- Enlaces o códigos de emparejamiento compartidos por error.
- Copias de seguridad expuestas.

## 6. Controles mínimos antes de uso real

- HTTPS obligatorio para servicios remotos.
- Contraseñas con hash resistente y sal única si se usa autenticación por contraseña.
- Tokens de corta duración y rotación/revocación.
- Límites de intentos y protección contra abuso.
- Autorización comprobada en el servidor para cada recurso.
- Cifrado de datos almacenados y gestión de claves fuera del repositorio.
- Registro de eventos de seguridad sin incluir secretos ni contenido privado.
- Pruebas de permisos, revocación, recuperación y acceso cruzado entre cuentas.
- Revisión independiente antes de almacenar datos personales reales.

## 7. Decisiones pendientes

- Autenticación por contraseña, passkeys o ambas.
- Backend y base de datos.
- Gestión y recuperación de claves.
- Modelo de cifrado de extremo a extremo y recuperación de cuenta.
- Política de retención, exportación y borrado.
- Marco legal y privacidad aplicable según países y edades.

## 8. Estado real

El backend 0.1 implementa registro e inicio de sesión, sesiones opacas revocables y alta/listado/revocación de dispositivos sobre PostgreSQL. El código está guardado en `backend/`, pero aún no se ha ejecutado ni probado con una base de datos real y no está desplegado. No hay sincronización ni almacenamiento remoto de archivos. No introducir credenciales ni datos personales reales en este prototipo.

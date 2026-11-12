# E.D-OS

Proyecto experimental para construir, por etapas, un sistema operativo y ecosistema tecnológico propios.

## Estado actual

- Escritorio web experimental en `index.html`, con gestor de ventanas múltiples en `js/window-manager.js`, búsqueda local y centro de notificaciones en `js/system-tools.js` (pendiente de pruebas reales).
- Aplicación experimental de archivos locales en `apps/archivos.html`.
- E.D. Freedom en `js/freedom.js`: búsquedas web, apertura directa de dominios, accesos rápidos a sitios conocidos y enlace alternativo si el navegador bloquea una ventana emergente. Los inicios de sesión ocurren en el sitio oficial dentro del navegador; no se almacenan contraseñas ni se evitan restricciones del dispositivo. Pendiente de pruebas reales.
- Aplicación de preferencias locales en `apps/ajustes.html` (tema, tamaño de texto, movimiento y reloj).
- Prueba WebRTC de conexión directa en `connect.html`; todavía no verificada entre dos dispositivos reales.
- Arquitectura inicial y hoja de ruta en `docs/`.

**Esto todavía no es un sistema operativo instalable ni una plataforma segura para datos personales.** El prototipo web no sustituye al kernel, los servicios del sistema, los controladores ni las aplicaciones nativas.

- Pantalla de acceso experimental en el escritorio: PIN de tres cifras, un intento por turno y bloqueo de 5 minutos tras el primer fallo y 10 minutos tras el segundo. El estado de bloqueo se guarda en el navegador. **No es seguridad real**: el PIN está en el JavaScript público y el bloqueo puede eliminarse borrando los datos del navegador o modificando el código. No usar para proteger información sensible.

## Backend de E.D. Ecosystem

- Servicio experimental en `backend/`: cuentas, sesiones revocables, inventario de dispositivos, emparejamiento temporal aprobado y sincronización versionada de registros JSON pequeños.
- Instrucciones y rutas disponibles: [backend/README.md](backend/README.md).
- **No está desplegado ni probado contra PostgreSQL ni entre dispositivos reales.** No usar con datos personales reales. Faltan verificación y recuperación de cuenta, cliente de emparejamiento, cifrado de extremo a extremo, sincronización de archivos, pruebas de seguridad y revisión independiente.

## Documentación

- [Arquitectura inicial](docs/ARQUITECTURA_INICIAL.md)
- [Plan de desarrollo 0.2](docs/PLAN_DESARROLLO_0.2.md)
- [Cuentas y seguridad](docs/CUENTAS_Y_SEGURIDAD.md)
- [Prueba experimental de E.D. Connect](docs/CONNECT_WEBRTC_TEST.md)

## Desarrollo

Rama de trabajo actual: `arquitectura/0.1`. Mantener `main` sin cambios durante esta etapa.

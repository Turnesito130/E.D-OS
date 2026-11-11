# E.D-OS

Proyecto experimental para construir, por etapas, un sistema operativo y ecosistema tecnológico propios.

## Estado actual

- Escritorio web experimental en `index.html`, con gestor de ventanas múltiples en `js/window-manager.js`, búsqueda local y centro de notificaciones en `js/system-tools.js` (pendiente de pruebas reales).
- Aplicación experimental de archivos locales en `apps/archivos.html`.
- Interfaz inicial de E.D. Freedom en `js/freedom.js`: búsquedas web externas y apertura de enlaces HTTP/HTTPS; no evita restricciones del navegador ni instala aplicaciones.
- Aplicación de preferencias locales en `apps/ajustes.html` (tema, tamaño de texto, movimiento y reloj).
- Prueba WebRTC de conexión directa en `connect.html`; todavía no verificada entre dos dispositivos reales.
- Arquitectura inicial y hoja de ruta en `docs/`.

**Esto todavía no es un sistema operativo instalable ni una plataforma segura para datos personales.** El prototipo web no sustituye al kernel, los servicios del sistema, los controladores ni las aplicaciones nativas.

## Documentación

- [Arquitectura inicial](docs/ARQUITECTURA_INICIAL.md)
- [Plan de desarrollo 0.2](docs/PLAN_DESARROLLO_0.2.md)
- [Cuentas y seguridad](docs/CUENTAS_Y_SEGURIDAD.md)
- [Prueba experimental de E.D. Connect](docs/CONNECT_WEBRTC_TEST.md)

## Desarrollo

Rama de trabajo actual: `arquitectura/0.1`. Mantener `main` sin cambios durante esta etapa.

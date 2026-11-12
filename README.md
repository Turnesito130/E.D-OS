# E.D-OS

Proyecto experimental para construir, por etapas, un sistema operativo y ecosistema tecnológico propios.

## Estado actual

- Escritorio web experimental en `index.html`, con gestor de ventanas múltiples en `js/window-manager.js`, búsqueda local y centro de notificaciones en `js/system-tools.js` (pendiente de pruebas reales).
- Aplicación experimental de archivos locales en `apps/archivos.html`.
- E.D. Freedom en `js/freedom.js`: búsquedas web, apertura directa de dominios, accesos rápidos a sitios conocidos y enlace alternativo si el navegador bloquea una ventana emergente. Los inicios de sesión ocurren en el sitio oficial dentro del navegador; no se almacenan contraseñas ni se evitan restricciones del dispositivo. Pendiente de pruebas reales.
- Aplicación de preferencias locales en `apps/ajustes.html` (tema, tamaño de texto, movimiento y reloj).
- Prueba WebRTC de conexión directa en `connect.html`; todavía no verificada entre dos dispositivos reales.

**Esto todavía no es un sistema operativo completo, instalable ni una plataforma segura para datos personales.** El escritorio web no sustituye a los servicios del sistema, controladores ni aplicaciones nativas.

## Núcleo propio

- [Arquitectura del núcleo E.D-OS](docs/ARQUITECTURA_NUCLEO_ED_OS.md)
- [Código y pruebas del kernel](kernel/README.md)

E.D-OS tendrá kernel propio y no dependerá del kernel Linux para arrancar ni funcionar. Para el primer prototipo se ha integrado Limine como cargador externo; Limine entrega el control al ELF del kernel E.D-OS, no sustituye al núcleo.

La compilación local del ELF x86_64 está verificada. La construcción de ISO híbrida y la prueba de arranque en QEMU están automatizadas en GitHub Actions y pendientes de resultado. Hasta que el log de QEMU contenga `KERNEL_BOOT_OK`, no se considera demostrado el arranque.

## Backend de E.D. Ecosystem

- Servicio experimental en `backend/`: cuentas, sesiones, inventario de dispositivos, emparejamiento temporal aprobado y sincronización versionada de registros JSON pequeños. Interfaz cliente experimental en `apps/ecosystem.html`.
- Instrucciones y rutas disponibles: [backend/README.md](backend/README.md).
- **No está desplegado ni probado contra PostgreSQL ni entre dispositivos reales.** No usar con datos personales reales. Faltan verificación y recuperación de cuenta, cliente de emparejamiento, cifrado de extremo a extremo, sincronización de archivos, pruebas de seguridad y revisión independiente.

## Desarrollo

Rama de trabajo: `arquitectura/0.1`. Mantener `main` sin cambios durante esta etapa.

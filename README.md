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

La compilación del ELF x86_64, las pruebas host de teclado, paginación, buddy, cambio de contexto, scheduler, IPC, syscalls, RAMFS, VFS y dispositivos de bloques están automatizadas en GitHub Actions. La última ejecución completa confirmada pasó en `arquitectura/0.1` (commit `8e673d83`): compilación ELF/ISO y arranque BIOS en QEMU. El kernel comprobó PMM, mapeo/desmapeo sobre el CR3 activo, ticks IRQ0, cambio cooperativo entre dos tareas, operaciones VFS y lectura/escritura sectorial en un ramdisk. No demuestra todavía multitarea preemptiva, modo usuario, aislamiento de procesos, almacenamiento físico SATA/NVMe ni arranque UEFI.

## Backend de E.D. Ecosystem

- Servicio experimental en `backend/`: cuentas, sesiones, inventario de dispositivos, emparejamiento temporal aprobado, sincronización versionada de registros JSON pequeños y controles de seguridad de cuenta (sesiones, cambio de contraseña e historial de eventos). La interfaz `apps/ecosystem.html` incorpora un panel de Seguridad y usuarios.
- Instrucciones y rutas disponibles: [backend/README.md](backend/README.md).
- **No está desplegado como servicio público para usuarios ni verificado entre dispositivos físicos.** El workflow de backend incluye pruebas de integración con PostgreSQL y almacenamiento S3 simulado; no usar con datos personales reales. Faltan verificación y recuperación de cuenta, MFA/passkeys, cifrado de extremo a extremo y revisión de seguridad independiente.

## Desarrollo

Rama de trabajo: `arquitectura/0.1`. Mantener `main` sin cambios durante esta etapa.


## Mejoras generales del escritorio (0.1)

- El acceso local ya no usa un PIN fijo publicado en el código. En el primer uso se configura un PIN de 6–12 cifras; el verificador se deriva con PBKDF2-SHA-256 y una sal aleatoria, y el bloqueo aplica esperas progresivas.
- Este control solo protege la apertura casual de esta página en el navegador. No es autenticación del kernel, no cifra los datos del perfil y puede eludirse manipulando el almacenamiento del navegador o el código cliente. No debe usarse como frontera de seguridad para datos sensibles.
- El panel «Estado del sistema» informa de conexión, contexto seguro, estimación de almacenamiento web y memoria JavaScript cuando el navegador expone esa métrica. Los datos no disponibles se muestran como no compatibles, no como cero.
- La aplicación Terminal ofrece `help`, `clear`, `date`, `status`, `open`, `echo`, `history` y `about`. Es un intérprete de órdenes del escritorio web: no ejecuta binarios, procesos del kernel ni comandos del dispositivo.
- El workflow `.github/workflows/frontend-check.yml` comprueba la sintaxis de los módulos JavaScript en cada cambio relevante.

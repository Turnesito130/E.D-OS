# E.D-OS — Arquitectura del núcleo nativo (borrador 0.1)

## Objetivo

Construir una plataforma de sistema operativo propia aprovechando componentes
maduros del kernel Linux, adaptándolos de forma gradual y manteniendo una
separación clara entre código upstream y código específico de E.D-OS.

E.D-OS no será una distribución genérica con un tema visual cambiado. La
identidad propia se desarrollará en la configuración, servicios de sistema,
seguridad, experiencia, ciclo de actualizaciones, runtime de aplicaciones y
servicios de ecosistema. El kernel Linux será inicialmente una base técnica
reutilizada y modificada, no un desarrollo desde cero.

## Decisiones iniciales

- **Base:** kernel Linux upstream, preferiblemente una rama estable/LTS
  mantenida.
- **Primer objetivo de arranque:** x86_64 en QEMU, con firmware UEFI/OVMF.
- **Primer hardware físico:** PC x86_64 UEFI seleccionado tras comprobar
  firmware, GPU, red, almacenamiento y periféricos.
- **ARM64:** objetivo posterior para dispositivos móviles; no prometer
  compatibilidad universal.
- **Método de personalización:** configuración propia + parches pequeños,
  documentados y probados. Evitar un fork masivo difícil de mantener.
- **Código fuente:** no duplicar el árbol completo dentro del repositorio de
  aplicaciones. Registrar upstream exacto, commit, licencia y checksums en un
  manifiesto cuando se seleccione la versión.
- **Integración:** mantener los servicios de usuario de E.D-OS separados del
  kernel siempre que sea posible.

## Capas propuestas

1. **Firmware y arranque:** UEFI, cargador de arranque y verificación de la
   cadena de arranque.
2. **Kernel:** Linux configurado para el hardware objetivo; gestión de CPU,
   memoria virtual, procesos, interrupciones, energía y drivers.
3. **Base de usuario:** init/supervisor, servicios esenciales, registro de
   eventos, gestión de dispositivos y red.
4. **Seguridad E.D.:** usuarios, capacidades, aislamiento, políticas MAC,
   sandboxing, gestión de claves y actualizaciones firmadas.
5. **Plataforma E.D.:** gestor de paquetes/aplicaciones, runtime, interfaz,
   accesibilidad y herramientas del sistema.
6. **Ecosistema:** E.D. Connect, cuenta, sincronización y E.D. Cloud. La
   conectividad cloud no será requisito para arrancar ni para usar el equipo.
7. **E.D. AI:** servicio de IA integrado con permisos explícitos; no obtiene
   acceso implícito a todos los datos o dispositivos.

## Plan de implementación

### N0 — Preparación verificable
- [ ] Elegir y fijar una versión estable/LTS mantenida.
- [ ] Registrar commit, origen, licencia y checksum.
- [ ] Definir toolchain y entorno de compilación reproducible.
- [ ] Crear configuración mínima x86_64 para QEMU.

### N1 — Kernel de referencia
- [ ] Obtener el código upstream sin modificaciones.
- [ ] Compilar el kernel de referencia.
- [ ] Arrancarlo en QEMU con una imagen de prueba.
- [ ] Guardar logs, configuración y pasos exactos de reproducción.

### N2 — Personalización E.D-OS
- [ ] Añadir fragmentos de configuración propios.
- [ ] Definir la primera modificación de bajo riesgo y su justificación.
- [ ] Mantener el cambio como parche pequeño y revisable.
- [ ] Recompilar y repetir las pruebas de arranque.

### N3 — Servicios nativos
- [ ] Elegir init/supervisor y diseñar el arranque de servicios.
- [ ] Crear gestión de dispositivos, logs y configuración.
- [ ] Diseñar permisos y aislamiento antes de ejecutar aplicaciones de terceros.

### N4 — Prototipo instalable
- [ ] Crear root filesystem mínimo.
- [ ] Integrar cargador, kernel y servicios.
- [ ] Probar instalación, actualización y recuperación en máquina virtual.
- [ ] Validar hardware físico antes de anunciar compatibilidad.

## Qué no haremos todavía

- No modificaremos gestión de memoria, scheduler ni drivers sin una necesidad
  demostrada y pruebas específicas.
- No copiaremos el árbol completo de Linux a mano ni cambiaremos nombres para
  presentarlo como código original.
- No afirmaremos que existe un kernel E.D-OS arrancable hasta compilarlo y
  probarlo.
- No usaremos la API experimental actual como sustituto de servicios nativos
  del sistema.

## Licencias y procedencia

Linux se distribuye bajo GPL-2.0-only, con avisos y condiciones que deben
respetarse. Cada componente adicional puede tener su propia licencia. Antes
de redistribuir imágenes o binarios, se preparará un inventario de licencias,
avisos y código fuente correspondiente. El uso de asistencia de IA no elimina
la obligación de revisar el código ni de cumplir el proceso y la licencia
upstream.

## Criterio de finalización

Una fase solo se marca como terminada cuando existe evidencia reproducible:
comandos, versiones, configuración, logs y resultado de las pruebas. Un archivo
de planificación o un commit no demuestra por sí solo que el kernel compile o
arranque.

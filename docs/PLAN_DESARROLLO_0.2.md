# E.D-OS — Plan de desarrollo 0.2

**Rama:** `arquitectura/0.1`  
**Estado:** hoja de ruta; las fases no se consideran terminadas hasta superar sus pruebas.

## Objetivo

Pasar del escritorio visual inicial a una plataforma experimental con funciones verificables. La prioridad es construir una base pequeña, comprensible y segura antes de ampliar el número de aplicaciones o dispositivos compatibles. El gestor de ventanas de la primera iteración está guardado en código, pero aún necesita pruebas reales.

## Orden de trabajo

### Fase 0 — Escritorio y experiencia\n- [x] Añadir búsqueda local de aplicaciones, tareas y nombres de archivos.\n- [x] Añadir centro de notificaciones local con historial limitado.\n- [x] Añadir atajo Ctrl/⌘ + K para abrir la búsqueda.\n- [ ] Probar búsqueda y notificaciones en navegadores reales y tecnologías de asistencia.
- [x] Añadir gestor experimental de ventanas múltiples.
- [x] Abrir varias aplicaciones en ventanas independientes.
- [x] Incorporar minimizar, maximizar, restaurar, cerrar y mover ventanas.
- [ ] Probar ventanas y accesibilidad en navegadores reales y tamaños de pantalla distintos.
- [ ] Añadir gestión de foco, ciclo de ventanas y persistencia de posición.

### Fase A — Aplicaciones locales
- [x] Primera interfaz de E.D. Freedom para búsquedas web mediante buscadores externos.
- [x] Abrir enlaces HTTP/HTTPS para que el navegador gestione la descarga o visualización.
- [ ] Validar fuentes, seguridad de descargas y compatibilidad en un entorno instalable.
- [ ] Crear gestor de descargas con historial y permisos en la plataforma nativa.
- [ ] Diseñar catálogo e instalación de aplicaciones verificadas.

**Límite de E.D. Freedom 0.1:** es una interfaz que delega las búsquedas a buscadores externos y abre enlaces en el navegador. No elimina bloqueos de ChromeOS, no descarga en segundo plano, no inspecciona archivos, no instala software y no concede permisos del sistema.
- [x] Primera aplicación experimental de Archivos.
- [ ] Probar creación, edición, importación, descarga y borrado en navegadores reales.
- [ ] Añadir pruebas de límites de almacenamiento, nombres, errores y recuperación.
- [x] Crear Ajustes con preferencias persistentes en el navegador.\n- [ ] Probar cambios de preferencias y persistencia en navegadores reales.
- [ ] Definir una interfaz común para abrir aplicaciones y comunicar errores.

**Límite actual de Archivos:** guarda texto y metadatos en el almacenamiento del navegador. No accede al disco del sistema, no es un gestor de archivos nativo y no sincroniza entre dispositivos. Exportar copia es importante porque los datos pueden perderse si se borra el almacenamiento del navegador.

### Fase B — E.D. Connect
- [ ] Probar WebRTC DataChannel entre dos dispositivos y redes distintas.
- [ ] Corregir la negociación ICE, desconexiones, reintentos y errores.
- [ ] Diseñar señalización para evitar copiar y pegar SDP.
- [ ] Incorporar autenticación y emparejamiento explícito de dispositivos.
- [ ] Evaluar servidor TURN para redes donde la conexión directa no funcione.
- [ ] Después de validar el canal, diseñar transferencia de archivos con límites, confirmación y permisos.

No llamar a esta función Bluetooth ni sincronización hasta que esas capacidades existan y hayan sido probadas.

### Fase C — Cuentas y sincronización
- [x] Implementar registro, inicio/cierre de sesión y consulta de cuenta (API experimental).\n- [ ] Añadir verificación de correo, recuperación de cuenta y MFA/passkeys.
- [x] Implementar registro, listado y revocación básica de dispositivos en la API experimental.\n- [x] Implementar solicitudes de emparejamiento temporales con aprobación autenticada y canje de sesión de un solo uso (API, aún sin cliente ni pruebas reales).
- [ ] Separar datos locales, datos sincronizados y datos compartidos.
- [ ] Definir cifrado en tránsito y protección de datos almacenados.
- [ ] Diseñar resolución de conflictos y borrado sincronizado.
- [ ] Probar la API con PostgreSQL y dos clientes reales; no sincronizar datos personales reales sin revisión de seguridad.

Una cuenta no debe dar permiso automático para acceder a todos los dispositivos. Cada dispositivo y cada tipo de dato requiere autorización.

### Fase D — Seguridad y plataforma
- [ ] Modelo de amenazas y límites de confianza.
- [ ] Modelo de permisos por aplicación.
- [ ] Gestión segura de sesiones y secretos.
- [ ] Política de actualizaciones verificables y recuperación ante fallos.
- [ ] Elegir hardware y base técnica para un prototipo instalable.
- [ ] Evaluar aislamiento de aplicaciones, servicios del sistema y compatibilidad.

## Criterios de calidad

Cada entrega debe incluir:
1. Qué se ha implementado y qué no.
2. Cómo reproducir la prueba.
3. Resultado de las comprobaciones automáticas.
4. Resultado de pruebas reales, o indicación explícita de que aún no se han realizado.
5. Riesgos, límites y siguiente paso.

## Decisiones que siguen abiertas

- Plataforma inicial y distribución base.
- Entorno gráfico y toolkit.
- Arquitectura de cuentas y backend.
- Servicio de señalización y TURN.
- Alcance de la primera versión instalable.

## Regla

Un commit confirma que el código se ha guardado en GitHub; no demuestra por sí solo que funcione en el dispositivo del usuario.
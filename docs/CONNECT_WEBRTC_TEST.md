# E.D. Connect — prueba de conexión directa 0.1

## Qué se ha implementado

La página `connect.html` contiene una primera prueba experimental de WebRTC DataChannel. Permite que dos navegadores intenten abrir un canal de datos directo mediante un intercambio manual de descripciones SDP (códigos largos copiados entre dispositivos).

- El dispositivo iniciador crea una oferta.
- El segundo dispositivo pega la oferta y genera una respuesta.
- El iniciador pega la respuesta.
- Si la negociación de red funciona, se abre un canal de datos cifrado por DTLS y se pueden intercambiar mensajes de texto de prueba.

La configuración usa un servidor STUN público de Google para descubrir candidatos de conexión. No se ha incorporado un servidor TURN ni un servicio propio de señalización.

## Cómo probarlo cuando la página esté publicada en HTTPS

1. Abrir `connect.html` en dos dispositivos y navegadores compatibles con WebRTC.
2. En el dispositivo A, seleccionar «Este será el dispositivo que inicia» y pulsar «Crear código de conexión».
3. Copiar el código local de A y pasarlo al dispositivo B por un medio privado.
4. En B, seleccionar «Este será el segundo dispositivo», pegar el código de A en «Código del otro dispositivo» y pulsar «Usar código recibido».
5. B generará una respuesta. Copiar el código de B y pasarlo a A.
6. En A, pegar la respuesta y pulsar «Usar código recibido».
7. Si el estado indica que la conexión está abierta, probar el envío de mensajes.

## Límites y seguridad

- Esto no es Bluetooth, no descubre dispositivos automáticamente y no sincroniza archivos.
- Ambos dispositivos necesitan Internet para el descubrimiento ICE/STUN y deben poder alcanzar una ruta compatible. Algunas redes, cortafuegos o NAT pueden impedir la conexión directa.
- Si la conexión falla por restricciones de red, hará falta desplegar un servicio TURN. Para evitar el intercambio manual de códigos, también hará falta un servicio de señalización.
- WebRTC cifra el transporte mediante DTLS, pero el intercambio manual de SDP no autentica por sí solo la identidad de la otra persona. Compartir los códigos solo por un canal privado y aceptar únicamente a un dispositivo de confianza.
- No se debe enviar información sensible en esta prueba.
- La página necesita un contexto seguro HTTPS (por ejemplo, GitHub Pages) o localhost para las funciones web pertinentes.
- El código está guardado en GitHub, pero todavía no se ha ejecutado ni probado en dos dispositivos reales. La compatibilidad y el éxito de conexión siguen pendientes de verificación.

## Próximos pasos técnicos

1. Probar la negociación entre dos navegadores en redes distintas y en la misma red.
2. Corregir errores observados y documentar compatibilidad.
3. Diseñar señalización propia y autenticación de dispositivos antes de convertirlo en una función estable.
4. Evaluar TURN y luego transferencia de archivos con límites, confirmación y permisos.

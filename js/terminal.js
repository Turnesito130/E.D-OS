'use strict';

(() => {
  const allowedApps = new Set(['Inicio', 'Archivos', 'Ajustes', 'Navegador', 'E.D. Connect', 'E.D. Ecosystem', 'E.D. Freedom', 'Buscar', 'Notificaciones']);
  const help = [
    'Comandos disponibles:',
    '  help                 muestra esta ayuda',
    '  clear                limpia la pantalla',
    '  date                 muestra fecha y hora',
    '  status               muestra el estado básico del navegador',
    '  open <aplicación>    abre una aplicación de E.D-OS',
    '  echo <texto>         imprime texto',
    '  history              muestra los últimos comandos',
    '  about                información del terminal'
  ];
  const history = [];
  const HISTORY_LIMIT = 100;
  const OUTPUT_LIMIT = 300;
  const COMMAND_MAX_LENGTH = 512;

  function attach(win) {
    const output = win.querySelector('[data-terminal-output]');
    const form = win.querySelector('[data-terminal-form]');
    const input = form?.querySelector('input');
    if (!output || !form || !input) return;
    const line = (text, className = '') => {
      const item = document.createElement('div');
      item.textContent = text;
      if (className) item.className = className;
      output.append(item);
      while (output.children.length > OUTPUT_LIMIT) output.firstElementChild?.remove();
      output.scrollTop = output.scrollHeight;
    };
    const execute = raw => {
      const command = raw.trim();
      if (!command) return;
      if (command.length > COMMAND_MAX_LENGTH) {
        line("Comando demasiado largo (máximo " + COMMAND_MAX_LENGTH + " caracteres).");
        return;
      }
      history.push(command);
      if (history.length > HISTORY_LIMIT) history.shift();
      line('$ ' + command, 'window-note');
      const [verb, ...parts] = command.split(/\s+/);
      const argument = parts.join(' ');
      switch (verb.toLowerCase()) {
        case 'help': help.forEach(text => line(text)); break;
        case 'clear': output.replaceChildren(); break;
        case 'date': line(new Date().toLocaleString('es-ES')); break;
        case 'echo': line(argument); break;
        case 'history': history.slice(-20).forEach((item, index) => line((history.length - Math.min(20, history.length) + index + 1) + '  ' + item)); break;
        case 'about':
          line('E.D-OS Terminal 0.1 — intérprete de comandos del escritorio web.');
          line('No es todavía una shell del kernel ni ejecuta programas nativos.');
          break;
        case 'status':
          line('Red: ' + (navigator.onLine ? 'conectada (según el navegador)' : 'sin conexión'));
          line('Contexto seguro: ' + (window.isSecureContext ? 'sí' : 'no'));
          line('Almacenamiento: ' + (navigator.storage?.estimate ? 'API disponible' : 'API no disponible'));
          break;
        case 'open': {
          const name = argument.toLocaleLowerCase('es');
          const match = [...allowedApps].find(app => app.toLocaleLowerCase('es') === name);
          if (!match) line('Aplicación desconocida. Escribe "help" para ver los comandos.');
          else document.dispatchEvent(new CustomEvent('ed-os:open-app', { detail: { name: match } }));
          break;
        }
        default: line('Comando desconocido: ' + verb + '. Escribe "help".');
      }
    };
    form.addEventListener('submit', event => {
      event.preventDefault();
      execute(input.value);
      input.value = '';
      input.focus();
    });
    help.slice(0, 1).forEach(text => line(text));
    line('Escribe "help" para ver los comandos.');
    input.focus();
  }

  document.addEventListener('ed-os:window-opened', event => {
    if (event.detail?.name === 'Terminal') attach(event.detail.element);
  });
})();

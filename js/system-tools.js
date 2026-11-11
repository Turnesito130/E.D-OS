(() => {
  'use strict';
  const KEY = 'ed-os-notifications-v1';
  const MAX = 30;
  const read = () => {
    try {
      const value = JSON.parse(localStorage.getItem(KEY) || '[]');
      return Array.isArray(value) ? value.filter(x => x && typeof x.message === 'string').slice(0, MAX) : [];
    } catch (_) { return []; }
  };
  let items = read();
  const save = () => { try { localStorage.setItem(KEY, JSON.stringify(items.slice(0, MAX))); } catch (_) {} };
  const escapeText = value => String(value).replace(/[&<>"]/g, ch => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[ch]));
  function notify(message, kind = 'info') {
    const item = { id: Date.now().toString(36) + Math.random().toString(36).slice(2,6), message: String(message).slice(0,240), kind, time: Date.now() };
    items.unshift(item); items = items.slice(0, MAX); save();
    document.dispatchEvent(new CustomEvent('ed-os:notification-added', { detail: item }));
  }
  window.EDOS = Object.freeze({ notify });
  function openApp(name) { document.dispatchEvent(new CustomEvent('ed-os:open-app', { detail: { name } })); }
  function searchIndex(query) {
    const q = query.trim().toLocaleLowerCase('es');
    if (!q) return [];
    const entries = [
      ...['Inicio','Archivos','Navegador','Ajustes','E.D. Connect','Asistente E.D.','E.D. Freedom'].map(name => ({type:'Aplicación',title:name,detail:'Abrir aplicación',action:()=>openApp(name)})),
      ...[...document.querySelectorAll('#tasks .task')].map(row => ({type:'Tarea',title:row.querySelector('span')?.textContent || '',detail:row.querySelector('input')?.checked?'Completada':'Pendiente',action:()=>{row.scrollIntoView({behavior:'smooth',block:'center'});row.focus?.();}})),
      ...(() => { try { const docs=JSON.parse(localStorage.getItem('ed-os-files-v1')||'[]'); return Array.isArray(docs)?docs.filter(x=>x&&typeof x.name==='string').map(file=>({type:'Archivo',title:file.name,detail:'Guardado en este navegador',action:()=>openApp('Archivos')})):[]; } catch (_) { return []; } })()
    ];
    return entries.filter(entry => (entry.title+' '+entry.detail+' '+entry.type).toLocaleLowerCase('es').includes(q)).slice(0,25);
  }
  function renderSearch(root, initial = '') {
    const input = root.querySelector('[data-search-input]');
    const results = root.querySelector('[data-search-results]');
    if (!input || !results) return;
    function draw() {
      results.replaceChildren();
      const matches = searchIndex(input.value);
      if (!input.value.trim()) {
        const hint = document.createElement('p'); hint.className='window-note'; hint.textContent='Busca aplicaciones, tareas y archivos guardados en este navegador.'; results.append(hint); return;
      }
      if (!matches.length) {
        const empty=document.createElement('p'); empty.className='window-note'; empty.textContent='No se han encontrado coincidencias.'; results.append(empty); return;
      }
      matches.forEach(entry => {
        const button=document.createElement('button'); button.type='button'; button.className='system-result';
        const title=document.createElement('strong'); title.textContent=entry.title;
        const meta=document.createElement('small'); meta.textContent=entry.type+' · '+entry.detail;
        button.append(title,meta); button.addEventListener('click',entry.action); results.append(button);
      });
    }
    input.addEventListener('input',draw); input.addEventListener('keydown',event=>{if(event.key==='Enter'){results.querySelector('button')?.click();}});
    input.value=initial; draw(); input.focus();
  }
  function renderNotifications(root) {
    const list=root.querySelector('[data-notification-list]');
    if(!list)return;
    list.replaceChildren();
    const clearButton=root.querySelector('[data-clear-notifications]'); if(clearButton) clearButton.onclick=()=>{items=[];save();renderNotifications(root);};
    if(!items.length){const empty=document.createElement('p');empty.className='window-note';empty.textContent='No tienes notificaciones.';list.append(empty);return;}
    items.forEach(item=>{
      const row=document.createElement('article');row.className='notification-item';
      const message=document.createElement('p');message.textContent=item.message;
      const time=document.createElement('small');time.textContent=new Intl.DateTimeFormat('es-ES',{dateStyle:'short',timeStyle:'short'}).format(new Date(item.time));
      row.append(message,time);list.append(row);
    });
  }
  document.addEventListener('ed-os:window-opened', event => {
    const {name,element}=event.detail||{};
    if(name==='Buscar')renderSearch(element);
    if(name==='Notificaciones')renderNotifications(element);
  });
  document.addEventListener('ed-os:notification-added', () => {
    const root=document.querySelector('[data-app-window="Notificaciones"]');
    if(root)renderNotifications(root);
  });
  document.addEventListener('DOMContentLoaded', () => {
    const dock=document.querySelector('.dock');
    if(!dock)return;
    const separator=dock.querySelector('.sep');
    const search=document.createElement('button');search.className='app system-app';search.type='button';search.title='Buscar';search.setAttribute('aria-label','Buscar');search.dataset.app='Buscar';search.textContent='⌕';
    const bell=document.createElement('button');bell.className='app system-app';bell.type='button';bell.title='Notificaciones';bell.setAttribute('aria-label','Notificaciones');bell.dataset.app='Notificaciones';bell.textContent='♧';
    dock.insertBefore(search,separator);dock.insertBefore(bell,separator);
    search.addEventListener('click',()=>openApp('Buscar'));
    bell.addEventListener('click',()=>openApp('Notificaciones'));
    notify('E.D-OS se ha iniciado en este navegador. Tus datos siguen almacenados localmente.');
    document.addEventListener('keydown',event=>{
      if((event.ctrlKey||event.metaKey)&&event.key.toLowerCase()==='k'){event.preventDefault();openApp('Buscar');}
    });
    document.getElementById('tasks')?.addEventListener('change',event=>{if(event.target.matches('input[type=checkbox]'))notify(event.target.checked?'Tarea marcada como completada.':'Tarea marcada como pendiente.');});
  });
})();

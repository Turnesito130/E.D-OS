'use strict';

(() => {
  const root = document.getElementById('systemHealth');
  if (!root) return;

  const rows = new Map();
  function setRow(key, label, value, state) {
    let row = rows.get(key);
    if (!row) {
      row = document.createElement('div');
      row.className = 'device';
      const name = document.createElement('div');
      const strong = document.createElement('strong');
      const small = document.createElement('small');
      name.append(strong, small);
      const status = document.createElement('span');
      status.className = 'status';
      row.append(name, status);
      root.append(row);
      rows.set(key, { row, strong, small, status });
    }
    row.strong.textContent = label;
    row.small.textContent = value;
    row.status.textContent = state === 'ok' ? 'OK' : state === 'warn' ? '!' : '—';
    row.status.setAttribute('aria-label', state === 'ok' ? 'Disponible' : state === 'warn' ? 'Limitado' : 'No disponible');
  }

  function updateConnection() {
    const online = navigator.onLine;
    setRow('network', 'Conexión', online ? 'El navegador informa de conexión' : 'Sin conexión de red', online ? 'ok' : 'warn');
  }

  function updateSecurityContext() {
    const secure = window.isSecureContext === true;
    setRow('context', 'Contexto seguro', secure ? 'HTTPS / localhost' : 'HTTP no seguro', secure ? 'ok' : 'warn');
  }

  async function updateStorage() {
    if (!navigator.storage || typeof navigator.storage.estimate !== 'function') {
      setRow('storage', 'Almacenamiento', 'Estimación no compatible con este navegador', 'na');
      return;
    }
    try {
      const estimate = await navigator.storage.estimate();
      const usage = Number.isFinite(estimate.usage) ? estimate.usage : null;
      const quota = Number.isFinite(estimate.quota) ? estimate.quota : null;
      if (usage === null || quota === null || quota <= 0) {
        setRow('storage', 'Almacenamiento', 'El navegador no ha facilitado una cuota', 'na');
        return;
      }
      const percent = Math.min(100, Math.round((usage / quota) * 100));
      const format = bytes => bytes >= 1073741824
        ? (bytes / 1073741824).toFixed(2) + ' GB'
        : (bytes / 1048576).toFixed(1) + ' MB';
      setRow('storage', 'Almacenamiento web', format(usage) + ' usados de ' + format(quota) + ' (' + percent + '%)', percent >= 90 ? 'warn' : 'ok');
    } catch (_) {
      setRow('storage', 'Almacenamiento', 'No se pudo consultar la estimación', 'na');
    }
  }

  function updateMemory() {
    const memory = performance && performance.memory;
    if (!memory || !Number.isFinite(memory.usedJSHeapSize) || !Number.isFinite(memory.jsHeapSizeLimit)) {
      setRow('memory', 'Memoria del navegador', 'Métrica no expuesta por este navegador', 'na');
      return;
    }
    const used = (memory.usedJSHeapSize / 1048576).toFixed(1);
    const limit = (memory.jsHeapSizeLimit / 1048576).toFixed(1);
    const ratio = memory.usedJSHeapSize / memory.jsHeapSizeLimit;
    setRow('memory', 'Memoria JavaScript', used + ' MB de límite ' + limit + ' MB', ratio >= 0.9 ? 'warn' : 'ok');
  }

  updateConnection();
  updateSecurityContext();
  updateStorage();
  updateMemory();
  window.addEventListener('online', updateConnection);
  window.addEventListener('offline', updateConnection);
  window.addEventListener('storage', updateStorage);
  document.addEventListener('visibilitychange', () => {
    if (!document.hidden) { updateStorage(); updateMemory(); }
  });
})();

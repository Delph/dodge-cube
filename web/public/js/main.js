"use strict";

const API_URL = 'http://192.168.0.10/api';

async function request(url, payload) {
  const options = {
    method: 'POST'
  };
  if (payload !== undefined)
    options.body = payload;

  try {
    return await fetch(`${API_URL}/${url}`, options);
  }
  catch (error) {
    console.error(error);
  }
}

document.addEventListener('DOMContentLoaded', async function() {
  // old stuff
  [...document.querySelectorAll('input[type=button][data-method]')].forEach(button => {
    button.addEventListener('click', async function(e) {
      const {method, ...dataset} = e.target.dataset;

      const fd = new FormData();
      for (const [k, v] of Object.entries(dataset))
        fd.append(k, v);

      await fetch(`${API_URL}/${method}`, {method: 'POST', body: fd});
      document.querySelector(`[data-mode].active`)?.classList.remove('active');
      document.querySelector(`[data-mode=${e.target.dataset['mode']}`)?.classList.add('active');
    });
  });

  // new stuff
  {
    const response = await request('status');
    const status = await response.json();

    document.querySelector('#power').checked = status.on;
    console.log(document.querySelector('#power').checked)

    document.querySelector(`[data-mode=${status.mode}]`)?.classList.add('active');
  }

  document.querySelector('#power').addEventListener('change', async event => {
    const which = event.detail.checked ? 'on' : 'off';
    await request(which);
  });

  document.querySelector('colour-wheel').addEventListener('colour', async event => {
    const colour = parseInt(event.detail.substr(1), 16);
    const fd = new FormData();
    fd.append('colour', colour);
    await request('set-static-colour', fd);
  });
});

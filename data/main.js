"use strict";

const API_URL = 'http://192.168.0.10/api';

function setup_picker()
{
  const picker = document.querySelector('canvas#colour-picker');
  const context = picker.getContext('2d');

  const id = new ImageData(255, 100);
  for (let i = 0; i < id.data.length; i += 4)
  {
    const x = ((i / 4) % id.width) / id.width;
    const y = Math.floor((i / 4) / id.width) / id.height;

    const hsv = new HSV(x, Math.min(y * 2.0, 1.0), Math.min(2.0 - y * 2.0, 1.0));
    // const hsv = new HSV(x, y, 1.0);
    const rgba = hsv.to_rgba();
    id.data[i + 0] = rgba[0];
    id.data[i + 1] = rgba[1];
    id.data[i + 2] = rgba[2];
    id.data[i + 3] = rgba[3];
  }

  context.putImageData(id, 0, 0);

  picker.addEventListener('click', async function(e) {
    const rect = e.target.getBoundingClientRect();
    const x = e.clientX - rect.left;
    const y = e.clientY - rect.top;

    const pixel = context.getImageData(x, y, 1, 1).data;
    const fd = new FormData();
    fd.append('colour', pixel[0] << 16 | pixel[1] << 8 | pixel[2]);
    await fetch(`${API_URL}/set-static-colour`, {method: 'POST', body: fd});
  });
}

document.addEventListener('DOMContentLoaded', function() {
  [...document.querySelectorAll('input[type=button][data-method]')].forEach(button => {
    button.addEventListener('click', async function(e) {
      const {method, ...dataset} = e.target.dataset;

      const fd = new FormData();
      for (const [k, v] of Object.entries(dataset))
        fd.append(k, v);

      await fetch(`${API_URL}/${method}`, {method: 'POST', body: fd});

      if (method === 'set-colour-mode')
        document.querySelector('.picker').classList.toggle('show', dataset.mode === 'static');
    });
  });

  setup_picker();
});

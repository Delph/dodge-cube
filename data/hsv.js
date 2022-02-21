"use strict";

class HSV
{
  constructor(h, s, v)
  {
    this.h = h;
    this.s = s;
    this.v = v;
  }

  to_rgb()
  {
    const i = Math.floor(this.h * 6);
    const f = this.h * 6 - i;
    const p = this.v * (1 - this.s);
    const q = this.v * (1 - f * this.s);
    const t = this.v * (1 - (1 - f) * this.s);
    switch (i % 6)
    {
      case 0:
        return [
          Math.round(this.v * 255),
          Math.round(t * 255),
          Math.round(p * 255)
        ];
      case 1:
        return [
          Math.round(q * 255),
          Math.round(this.v * 255),
          Math.round(p * 255)
        ]
      case 2:
        return [
          Math.round(p * 255),
          Math.round(this.v * 255),
          Math.round(t * 255)
        ];
      case 3:
        return [
          Math.round(p * 255),
          Math.round(q * 255),
          Math.round(this.v * 255)
        ];
      case 4:
        return [
          Math.round(t * 255),
          Math.round(p * 255),
          Math.round(this.v * 255)
        ];
      case 5:
        return [
          Math.round(this.v * 255),
          Math.round(p * 255),
          Math.round(q * 255)
        ];
    }
    return [0, 0, 0]
  }

  to_rgba()
  {
    return [...this.to_rgb(), 0xFF];
  }
}

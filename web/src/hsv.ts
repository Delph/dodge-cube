export class HSV
{
  h: number;
  s: number;
  v: number;

  constructor(h: number, s: number, v: number)
  {
    this.h = h;
    this.s = s;
    this.v = v;
  }

  static rgb(r: number, g: number, b: number)
  {
    // Normalize RGB values to the range 0-1
    const rPrime = r / 255;
    const gPrime = g / 255;
    const bPrime = b / 255;

    // Calculate max and min values
    const max = Math.max(rPrime, gPrime, bPrime);
    const min = Math.min(rPrime, gPrime, bPrime);
    const delta = max - min;

    // Calculate Hue (H)
    let h = 0;
    if (delta !== 0)
    {
      if (max === rPrime)
        h = 60 * (((gPrime - bPrime) / delta) % 6);
      else if (max === gPrime)
        h = 60 * (((bPrime - rPrime) / delta) + 2);
      else if (max === bPrime)
        h = 60 * (((rPrime - gPrime) / delta) + 4);
    }
    if (h < 0)
      h += 360;

    const s = (max === 0) ? 0 : delta / max;
    const v = max;

    return new HSV(
      Math.round(h),
      Math.round(s * 100),
      Math.round(v * 100)
    );
  }

  to_rgb()
  {
    while (this.h < 0)
      this.h += 360;
    while (this.h >= 360)
      this.h -= 360;
    const i = Math.floor((this.h / 360) * 6);
    const f = (this.h / 360) * 6 - i;
    const p = this.v / 100 * (1 - this.s / 100);
    const q = this.v / 100 * (1 - f * this.s / 100);
    const t = this.v / 100 * (1 - (1 - f) * this.s / 100);
    switch (i % 6)
    {
      case 0:
        return [
          Math.round(this.v / 100 * 255),
          Math.round(t * 255),
          Math.round(p * 255)
        ];
      case 1:
        return [
          Math.round(q * 255),
          Math.round(this.v / 100 * 255),
          Math.round(p * 255)
        ]
      case 2:
        return [
          Math.round(p * 255),
          Math.round(this.v / 100 * 255),
          Math.round(t * 255)
        ];
      case 3:
        return [
          Math.round(p * 255),
          Math.round(q * 255),
          Math.round(this.v / 100 * 255)
        ];
      case 4:
        return [
          Math.round(t * 255),
          Math.round(p * 255),
          Math.round(this.v / 100 * 255)
        ];
      case 5:
        return [
          Math.round(this.v / 100 * 255),
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

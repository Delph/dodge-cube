import { Component } from "./Component.ts";
import { toKebabCase } from "./utility.ts";


export class ComponentLibrary {
  static _library = new Set<typeof Component>();

  static add(...components: typeof Component[]) {
    for (const component of components)
      ComponentLibrary._library.add(component);
  }

  static _register() {
    const names: string[] = [];
    for (const component of ComponentLibrary._library.values()) {
      const name = toKebabCase(component.name);
      if (names.includes(name))
        throw new Error(`A component with the name ${name} has already been registered for ${component.name}`);
      component._attributeList = component._getAttributes();
      customElements.define(name, component);
    }
  }
}

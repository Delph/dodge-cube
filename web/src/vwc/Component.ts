import { toKebabCase } from "./utility.ts";
import { Dep, watchEffect } from "./Reactive.ts";

type Binding = {
  /// the element's tag name (e.g., input)
  tagName: string;

  /// the attribute's name (e.g., type)
  attributeName: string;

  /// the binding value (e.g., $kind => type)
  binding: string;
};

export class Component extends HTMLElement {
  static _library = new Set<typeof Component>();
  static _templateCache = new Map();
  static _attributeList: Binding[] = [];

  protected static template?: string;

  bindingMap: {[name: string]: Binding & {element: Element}};

  static register(...components: typeof Component[]) {
    if (components.length === 0) {
      Component._library.add(this);
    }
    else {
      for (const component of components)
        Component._library.add(component);
    }
  }

  static _registerAll() {
    const names: string[] = [];
    for (const component of Component._library.values()) {
      const name = toKebabCase(component.name);
      if (names.includes(name))
        throw new Error(`A component with the name ${name} has already been registered for ${component.name}`);
      component._attributeList = component._getAttributes();
      customElements.define(name, component);
    }
  }

  static _getTemplate() {
    if (!Component._templateCache.has(this.name)) {
      if (this.template !== undefined) {
        const template = document.createElement('template');
        template.innerHTML = this.template;
        Component._templateCache.set(this.name, template);
      }
      else {
        const id = `template-${toKebabCase(this.name)}`;
        const template = document.getElementById(id);
        if (template === null)
          throw new Error(`No such template ${id} for ${this.name}`);
        Component._templateCache.set(this.name, template);
      }
    }
    return Component._templateCache.get(this.name);
  }

  static _getAttributes() {
    const template = this._getTemplate();
    if (!template)
      return [];

    const attributes = new Set<Binding>();
    const walker = document.createTreeWalker(template.content, NodeFilter.SHOW_ELEMENT);
    while (walker.nextNode()) {
      const element = walker.currentNode as HTMLElement;
      for (const attribute of element.attributes) {
        if (attribute.value.startsWith('$')) {
          attributes.add({
            tagName: element.tagName,
            attributeName: attribute.name,
            binding: attribute.value
          });
        }
      }
    }
    return Array.from(attributes);
  }


  constructor() {
    super();

    const template = (this.constructor as typeof Component)._getTemplate();
    const shadow = this.attachShadow({mode: 'open'});
    shadow.appendChild(template.content.cloneNode(true));

    this.bindingMap = {};
    this._initialiseAttributes();
  }

  get hasRoot() {
    return this.shadowRoot !== null;
  }

  get root() {
    return this.shadowRoot!;
  }

  static get observedAttributes() {
    const list = this._attributeList.map(a => a.binding.substring(1)) || [];
    console.log(`${this.name}: ${JSON.stringify(list)}`);
    return list;
  }

  attributeChangedCallback(name: string, oldValue: string, newValue: string) {
    console.debug(`attributeChangedCallback("${name}", "${oldValue}", "${newValue}"`);
    if (oldValue === newValue)
      return;

    this._updateAttribute(name, newValue);
    this._updateComputedProperties();
  }

  connectedCallback() {
    watchEffect(() => {
      this._updateComputedProperties();
    });
  }

  _initialiseAttributes() {
    for (const binding of (this.constructor as typeof Component)._attributeList) {
      const element = this.root.querySelector(`[${binding.attributeName}="${binding.binding}"]`);
      if (element) {
        this.bindingMap[binding.binding.substring(1)] = {...binding, element};
      }
    }
  }

  _updateAttribute(name: string, value: string) {
    if (!this.hasRoot)
      return;
    if (this.bindingMap[name] === undefined)
      return;
    this._setBinding(this.bindingMap[name], value);
  }

  _updateComputedProperties() {
    for (const key of Object.keys(this.bindingMap)) {
      if ((this as any)[key] !== undefined) {
        const value = (this as any)[key];
        const binding = this.bindingMap[key];
        this._setBinding(binding, value);
      }
    }
  }

  _watchReactiveProperties() {
    for (const key of Object.keys(this.bindingMap)) {
      if ((this as any)[key] instanceof Dep) {
        (this as any)[key].watch(() => this._updateComputedProperties());
      }
    }
  }

  _setBinding(binding: Binding & { element: Element }, value: string) {
    if (binding.tagName === 'SLOT')
      binding.element.innerHTML = value;
    else
      binding.element.setAttribute(binding.attributeName, value);
  }
}


document.addEventListener('DOMContentLoaded', Component._registerAll);

// LUMA Viewer Web - Main Entry Point
// WebGPU-based 3D model viewer with mouse/touch controls

import { LumaRenderer } from './renderer';
import { OrbitCamera } from './camera';
import { createCubeMesh, createGridMesh } from './geometry';

class LumaViewerApp {
    private canvas: HTMLCanvasElement;
    private renderer: LumaRenderer | null = null;
    private camera: OrbitCamera;
    
    // State
    private showGrid = true;
    private autoRotate = false;
    private lastFrameTime = 0;
    private frameCount = 0;
    private fps = 0;
    
    // Mouse/touch state
    private isDragging = false;
    private lastMouseX = 0;
    private lastMouseY = 0;
    private touchStartDistance = 0;
    
    constructor() {
        this.canvas = document.getElementById('render-canvas') as HTMLCanvasElement;
        this.camera = new OrbitCamera();
        
        this.setupEventListeners();
    }
    
    async init() {
        try {
            // Check WebGPU support
            if (!navigator.gpu) {
                this.showError('WebGPU is not supported in this browser');
                return;
            }
            
            // Initialize renderer
            this.renderer = new LumaRenderer(this.canvas);
            await this.renderer.init();
            
            // Create demo meshes
            const cubeMesh = createCubeMesh(this.renderer.device);
            const gridMesh = createGridMesh(this.renderer.device);
            this.renderer.setMeshes([cubeMesh], gridMesh);
            
            // Show UI
            this.hideLoading();
            
            // Start render loop
            this.lastFrameTime = performance.now();
            this.animate();
            
        } catch (error) {
            console.error('Initialization failed:', error);
            this.showError(`Initialization failed: ${error}`);
        }
    }
    
    private setupEventListeners() {
        // Resize
        window.addEventListener('resize', () => this.onResize());
        
        // Mouse events
        this.canvas.addEventListener('mousedown', (e) => this.onMouseDown(e));
        this.canvas.addEventListener('mousemove', (e) => this.onMouseMove(e));
        this.canvas.addEventListener('mouseup', () => this.onMouseUp());
        this.canvas.addEventListener('wheel', (e) => this.onWheel(e));
        this.canvas.addEventListener('contextmenu', (e) => e.preventDefault());
        
        // Touch events
        this.canvas.addEventListener('touchstart', (e) => this.onTouchStart(e));
        this.canvas.addEventListener('touchmove', (e) => this.onTouchMove(e));
        this.canvas.addEventListener('touchend', () => this.onTouchEnd());
        
        // Toolbar buttons
        document.getElementById('btn-load')?.addEventListener('click', () => this.loadModel());
        document.getElementById('btn-grid')?.addEventListener('click', () => this.toggleGrid());
        document.getElementById('btn-rotate')?.addEventListener('click', () => this.toggleAutoRotate());
        document.getElementById('btn-reset')?.addEventListener('click', () => this.resetCamera());
        
        // File input
        document.getElementById('file-input')?.addEventListener('change', (e) => this.onFileSelected(e));
    }
    
    private onResize() {
        const width = window.innerWidth;
        const height = window.innerHeight - 60; // Account for toolbar
        
        this.canvas.width = width * devicePixelRatio;
        this.canvas.height = height * devicePixelRatio;
        this.canvas.style.width = `${width}px`;
        this.canvas.style.height = `${height}px`;
        
        this.camera.aspectRatio = width / height;
        this.renderer?.resize(this.canvas.width, this.canvas.height);
    }
    
    private onMouseDown(e: MouseEvent) {
        this.isDragging = true;
        this.lastMouseX = e.clientX;
        this.lastMouseY = e.clientY;
    }
    
    private onMouseMove(e: MouseEvent) {
        if (!this.isDragging) return;
        
        const deltaX = e.clientX - this.lastMouseX;
        const deltaY = e.clientY - this.lastMouseY;
        
        if (e.buttons === 1) {
            // Left button - orbit
            this.camera.orbit(deltaX * 0.005, deltaY * 0.005);
        } else if (e.buttons === 2 || e.buttons === 4) {
            // Right/middle button - pan
            this.camera.pan(deltaX * 0.002, -deltaY * 0.002);
        }
        
        this.lastMouseX = e.clientX;
        this.lastMouseY = e.clientY;
    }
    
    private onMouseUp() {
        this.isDragging = false;
    }
    
    private onWheel(e: WheelEvent) {
        e.preventDefault();
        const delta = e.deltaY > 0 ? 1.1 : 0.9;
        this.camera.zoom(delta);
    }
    
    private onTouchStart(e: TouchEvent) {
        e.preventDefault();
        
        if (e.touches.length === 1) {
            this.isDragging = true;
            this.lastMouseX = e.touches[0].clientX;
            this.lastMouseY = e.touches[0].clientY;
        } else if (e.touches.length === 2) {
            const dx = e.touches[0].clientX - e.touches[1].clientX;
            const dy = e.touches[0].clientY - e.touches[1].clientY;
            this.touchStartDistance = Math.sqrt(dx * dx + dy * dy);
        }
    }
    
    private onTouchMove(e: TouchEvent) {
        e.preventDefault();
        
        if (e.touches.length === 1 && this.isDragging) {
            const deltaX = e.touches[0].clientX - this.lastMouseX;
            const deltaY = e.touches[0].clientY - this.lastMouseY;
            this.camera.orbit(deltaX * 0.005, deltaY * 0.005);
            this.lastMouseX = e.touches[0].clientX;
            this.lastMouseY = e.touches[0].clientY;
        } else if (e.touches.length === 2) {
            const dx = e.touches[0].clientX - e.touches[1].clientX;
            const dy = e.touches[0].clientY - e.touches[1].clientY;
            const distance = Math.sqrt(dx * dx + dy * dy);
            
            if (this.touchStartDistance > 0) {
                const scale = this.touchStartDistance / distance;
                this.camera.zoom(scale);
            }
            this.touchStartDistance = distance;
        }
    }
    
    private onTouchEnd() {
        this.isDragging = false;
        this.touchStartDistance = 0;
    }
    
    private loadModel() {
        document.getElementById('file-input')?.click();
    }
    
    private onFileSelected(e: Event) {
        const input = e.target as HTMLInputElement;
        const file = input.files?.[0];
        if (file) {
            console.log('Loading file:', file.name);
            // TODO: Implement model loading
            this.updateModelInfo(`Loaded: ${file.name}`);
        }
    }
    
    private toggleGrid() {
        this.showGrid = !this.showGrid;
        const btn = document.getElementById('btn-grid');
        btn?.classList.toggle('active', this.showGrid);
    }
    
    private toggleAutoRotate() {
        this.autoRotate = !this.autoRotate;
        const btn = document.getElementById('btn-rotate');
        btn?.classList.toggle('active', this.autoRotate);
    }
    
    private resetCamera() {
        this.camera.reset();
    }
    
    private animate() {
        requestAnimationFrame(() => this.animate());
        
        // Calculate FPS
        const now = performance.now();
        const delta = now - this.lastFrameTime;
        this.frameCount++;
        
        if (delta >= 1000) {
            this.fps = (this.frameCount * 1000) / delta;
            this.frameCount = 0;
            this.lastFrameTime = now;
            this.updateFPS();
        }
        
        // Auto rotate
        if (this.autoRotate) {
            this.camera.orbit(0.005, 0);
        }
        
        // Update and render
        if (this.renderer) {
            this.renderer.setCamera(this.camera.getViewMatrix(), this.camera.getProjectionMatrix());
            this.renderer.setShowGrid(this.showGrid);
            this.renderer.render();
        }
    }
    
    private updateFPS() {
        const el = document.getElementById('fps-display');
        if (el) el.textContent = `FPS: ${Math.round(this.fps)}`;
    }
    
    private updateModelInfo(info: string) {
        const el = document.getElementById('model-info');
        if (el) el.textContent = info;
    }
    
    private hideLoading() {
        document.getElementById('loading')!.style.display = 'none';
        document.getElementById('canvas-container')!.style.display = 'block';
        document.getElementById('toolbar')!.style.display = 'flex';
        this.onResize();
    }
    
    private showError(message: string) {
        document.getElementById('loading')!.style.display = 'none';
        const errorPanel = document.getElementById('error-panel')!;
        errorPanel.style.display = 'block';
        errorPanel.querySelector('p')!.textContent = message;
    }
}

// Start application
const app = new LumaViewerApp();
app.init();

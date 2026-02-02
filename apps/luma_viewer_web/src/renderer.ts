// LUMA Viewer Web - WebGPU Renderer
// Hardware-accelerated 3D rendering using WebGPU

import { mat4, identity } from './math';
import { Mesh } from './geometry';

// Shaders
const shaderCode = `
struct Uniforms {
    viewProjection: mat4x4<f32>,
    model: mat4x4<f32>,
    color: vec4<f32>,
};

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) normal: vec3<f32>,
    @location(2) color: vec3<f32>,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec3<f32>,
    @location(1) normal: vec3<f32>,
};

@vertex
fn vertexMain(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    let worldPos = uniforms.model * vec4<f32>(input.position, 1.0);
    output.position = uniforms.viewProjection * worldPos;
    output.color = input.color;
    output.normal = (uniforms.model * vec4<f32>(input.normal, 0.0)).xyz;
    return output;
}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4<f32> {
    // Simple directional lighting
    let lightDir = normalize(vec3<f32>(0.5, 0.8, 0.3));
    let ambient = 0.3;
    let diffuse = max(dot(normalize(input.normal), lightDir), 0.0);
    let lighting = ambient + diffuse * 0.7;
    
    return vec4<f32>(input.color * lighting, 1.0);
}
`;

const gridShaderCode = `
struct Uniforms {
    viewProjection: mat4x4<f32>,
    model: mat4x4<f32>,
    color: vec4<f32>,
};

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

struct VertexInput {
    @location(0) position: vec3<f32>,
};

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
};

@vertex
fn vertexMain(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    output.position = uniforms.viewProjection * vec4<f32>(input.position, 1.0);
    return output;
}

@fragment
fn fragmentMain() -> @location(0) vec4<f32> {
    return uniforms.color;
}
`;

export class LumaRenderer {
    private canvas: HTMLCanvasElement;
    private context!: GPUCanvasContext;
    public device!: GPUDevice;
    private pipeline!: GPURenderPipeline;
    private gridPipeline!: GPURenderPipeline;
    private uniformBuffer!: GPUBuffer;
    private uniformBindGroup!: GPUBindGroup;
    private depthTexture!: GPUTexture;
    
    private meshes: Mesh[] = [];
    private gridMesh: Mesh | null = null;
    private showGrid = true;
    
    private viewMatrix: mat4 = identity();
    private projMatrix: mat4 = identity();
    
    constructor(canvas: HTMLCanvasElement) {
        this.canvas = canvas;
    }
    
    async init() {
        // Get adapter and device
        const adapter = await navigator.gpu.requestAdapter();
        if (!adapter) throw new Error('WebGPU adapter not found');
        
        this.device = await adapter.requestDevice();
        
        // Setup canvas context
        this.context = this.canvas.getContext('webgpu')!;
        const format = navigator.gpu.getPreferredCanvasFormat();
        
        this.context.configure({
            device: this.device,
            format,
            alphaMode: 'premultiplied',
        });
        
        // Create uniform buffer
        this.uniformBuffer = this.device.createBuffer({
            size: 16 * 4 * 3 + 4 * 4, // viewProj + model + color
            usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
        });
        
        // Create bind group layout
        const bindGroupLayout = this.device.createBindGroupLayout({
            entries: [{
                binding: 0,
                visibility: GPUShaderStage.VERTEX | GPUShaderStage.FRAGMENT,
                buffer: { type: 'uniform' },
            }],
        });
        
        // Create bind group
        this.uniformBindGroup = this.device.createBindGroup({
            layout: bindGroupLayout,
            entries: [{
                binding: 0,
                resource: { buffer: this.uniformBuffer },
            }],
        });
        
        // Create main pipeline
        const shaderModule = this.device.createShaderModule({ code: shaderCode });
        
        this.pipeline = this.device.createRenderPipeline({
            layout: this.device.createPipelineLayout({
                bindGroupLayouts: [bindGroupLayout],
            }),
            vertex: {
                module: shaderModule,
                entryPoint: 'vertexMain',
                buffers: [{
                    arrayStride: 9 * 4, // pos(3) + normal(3) + color(3)
                    attributes: [
                        { shaderLocation: 0, offset: 0, format: 'float32x3' },
                        { shaderLocation: 1, offset: 3 * 4, format: 'float32x3' },
                        { shaderLocation: 2, offset: 6 * 4, format: 'float32x3' },
                    ],
                }],
            },
            fragment: {
                module: shaderModule,
                entryPoint: 'fragmentMain',
                targets: [{ format }],
            },
            primitive: {
                topology: 'triangle-list',
                cullMode: 'back',
            },
            depthStencil: {
                depthWriteEnabled: true,
                depthCompare: 'less',
                format: 'depth24plus',
            },
        });
        
        // Create grid pipeline (lines)
        const gridShaderModule = this.device.createShaderModule({ code: gridShaderCode });
        
        this.gridPipeline = this.device.createRenderPipeline({
            layout: this.device.createPipelineLayout({
                bindGroupLayouts: [bindGroupLayout],
            }),
            vertex: {
                module: gridShaderModule,
                entryPoint: 'vertexMain',
                buffers: [{
                    arrayStride: 3 * 4,
                    attributes: [
                        { shaderLocation: 0, offset: 0, format: 'float32x3' },
                    ],
                }],
            },
            fragment: {
                module: gridShaderModule,
                entryPoint: 'fragmentMain',
                targets: [{ format }],
            },
            primitive: {
                topology: 'line-list',
            },
            depthStencil: {
                depthWriteEnabled: true,
                depthCompare: 'less',
                format: 'depth24plus',
            },
        });
        
        // Create depth texture
        this.createDepthTexture();
    }
    
    private createDepthTexture() {
        this.depthTexture = this.device.createTexture({
            size: [this.canvas.width, this.canvas.height],
            format: 'depth24plus',
            usage: GPUTextureUsage.RENDER_ATTACHMENT,
        });
    }
    
    resize(width: number, height: number) {
        this.depthTexture.destroy();
        this.createDepthTexture();
    }
    
    setCamera(view: mat4, projection: mat4) {
        this.viewMatrix = view;
        this.projMatrix = projection;
    }
    
    setMeshes(meshes: Mesh[], grid: Mesh | null = null) {
        this.meshes = meshes;
        this.gridMesh = grid;
    }
    
    setShowGrid(show: boolean) {
        this.showGrid = show;
    }
    
    render() {
        // Calculate view-projection matrix
        const viewProj = multiplyMat4(this.projMatrix, this.viewMatrix);
        
        const commandEncoder = this.device.createCommandEncoder();
        
        const renderPass = commandEncoder.beginRenderPass({
            colorAttachments: [{
                view: this.context.getCurrentTexture().createView(),
                clearValue: { r: 0.1, g: 0.1, b: 0.15, a: 1 },
                loadOp: 'clear',
                storeOp: 'store',
            }],
            depthStencilAttachment: {
                view: this.depthTexture.createView(),
                depthClearValue: 1.0,
                depthLoadOp: 'clear',
                depthStoreOp: 'store',
            },
        });
        
        // Draw grid
        if (this.showGrid && this.gridMesh) {
            renderPass.setPipeline(this.gridPipeline);
            renderPass.setBindGroup(0, this.uniformBindGroup);
            
            // Update uniforms for grid
            const gridUniformData = new Float32Array(16 + 16 + 4);
            gridUniformData.set(viewProj, 0);
            gridUniformData.set(identity(), 16);
            gridUniformData.set([0.3, 0.3, 0.3, 1.0], 32);
            this.device.queue.writeBuffer(this.uniformBuffer, 0, gridUniformData);
            
            renderPass.setVertexBuffer(0, this.gridMesh.vertexBuffer);
            renderPass.draw(this.gridMesh.vertexCount);
        }
        
        // Draw meshes
        renderPass.setPipeline(this.pipeline);
        renderPass.setBindGroup(0, this.uniformBindGroup);
        
        for (const mesh of this.meshes) {
            // Update uniforms
            const uniformData = new Float32Array(16 + 16 + 4);
            uniformData.set(viewProj, 0);
            uniformData.set(identity(), 16);
            uniformData.set([1, 1, 1, 1], 32);
            this.device.queue.writeBuffer(this.uniformBuffer, 0, uniformData);
            
            renderPass.setVertexBuffer(0, mesh.vertexBuffer);
            if (mesh.indexBuffer) {
                renderPass.setIndexBuffer(mesh.indexBuffer, 'uint32');
                renderPass.drawIndexed(mesh.indexCount);
            } else {
                renderPass.draw(mesh.vertexCount);
            }
        }
        
        renderPass.end();
        
        this.device.queue.submit([commandEncoder.finish()]);
    }
}

// Matrix multiplication helper
function multiplyMat4(a: mat4, b: mat4): mat4 {
    const result: mat4 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    
    for (let i = 0; i < 4; i++) {
        for (let j = 0; j < 4; j++) {
            result[i * 4 + j] = 
                a[i * 4 + 0] * b[0 * 4 + j] +
                a[i * 4 + 1] * b[1 * 4 + j] +
                a[i * 4 + 2] * b[2 * 4 + j] +
                a[i * 4 + 3] * b[3 * 4 + j];
        }
    }
    
    return result;
}

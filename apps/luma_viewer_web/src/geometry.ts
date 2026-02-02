// LUMA Viewer Web - Geometry Utilities
// Mesh creation and management

export interface Mesh {
    vertexBuffer: GPUBuffer;
    indexBuffer?: GPUBuffer;
    vertexCount: number;
    indexCount: number;
}

export function createCubeMesh(device: GPUDevice): Mesh {
    // Cube vertices: position (3) + normal (3) + color (3)
    const vertices = new Float32Array([
        // Front face (red)
        -0.5, -0.5,  0.5,   0, 0, 1,   0.8, 0.2, 0.2,
         0.5, -0.5,  0.5,   0, 0, 1,   0.8, 0.2, 0.2,
         0.5,  0.5,  0.5,   0, 0, 1,   0.8, 0.2, 0.2,
        -0.5,  0.5,  0.5,   0, 0, 1,   0.8, 0.2, 0.2,
        // Back face (green)
        -0.5, -0.5, -0.5,   0, 0,-1,   0.2, 0.8, 0.2,
        -0.5,  0.5, -0.5,   0, 0,-1,   0.2, 0.8, 0.2,
         0.5,  0.5, -0.5,   0, 0,-1,   0.2, 0.8, 0.2,
         0.5, -0.5, -0.5,   0, 0,-1,   0.2, 0.8, 0.2,
        // Top face (blue)
        -0.5,  0.5, -0.5,   0, 1, 0,   0.2, 0.2, 0.8,
        -0.5,  0.5,  0.5,   0, 1, 0,   0.2, 0.2, 0.8,
         0.5,  0.5,  0.5,   0, 1, 0,   0.2, 0.2, 0.8,
         0.5,  0.5, -0.5,   0, 1, 0,   0.2, 0.2, 0.8,
        // Bottom face (yellow)
        -0.5, -0.5, -0.5,   0,-1, 0,   0.8, 0.8, 0.2,
         0.5, -0.5, -0.5,   0,-1, 0,   0.8, 0.8, 0.2,
         0.5, -0.5,  0.5,   0,-1, 0,   0.8, 0.8, 0.2,
        -0.5, -0.5,  0.5,   0,-1, 0,   0.8, 0.8, 0.2,
        // Right face (cyan)
         0.5, -0.5, -0.5,   1, 0, 0,   0.2, 0.8, 0.8,
         0.5,  0.5, -0.5,   1, 0, 0,   0.2, 0.8, 0.8,
         0.5,  0.5,  0.5,   1, 0, 0,   0.2, 0.8, 0.8,
         0.5, -0.5,  0.5,   1, 0, 0,   0.2, 0.8, 0.8,
        // Left face (magenta)
        -0.5, -0.5, -0.5,  -1, 0, 0,   0.8, 0.2, 0.8,
        -0.5, -0.5,  0.5,  -1, 0, 0,   0.8, 0.2, 0.8,
        -0.5,  0.5,  0.5,  -1, 0, 0,   0.8, 0.2, 0.8,
        -0.5,  0.5, -0.5,  -1, 0, 0,   0.8, 0.2, 0.8,
    ]);
    
    const indices = new Uint32Array([
        0, 1, 2, 0, 2, 3,       // Front
        4, 5, 6, 4, 6, 7,       // Back
        8, 9, 10, 8, 10, 11,    // Top
        12, 13, 14, 12, 14, 15, // Bottom
        16, 17, 18, 16, 18, 19, // Right
        20, 21, 22, 20, 22, 23  // Left
    ]);
    
    const vertexBuffer = device.createBuffer({
        size: vertices.byteLength,
        usage: GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST,
    });
    device.queue.writeBuffer(vertexBuffer, 0, vertices);
    
    const indexBuffer = device.createBuffer({
        size: indices.byteLength,
        usage: GPUBufferUsage.INDEX | GPUBufferUsage.COPY_DST,
    });
    device.queue.writeBuffer(indexBuffer, 0, indices);
    
    return {
        vertexBuffer,
        indexBuffer,
        vertexCount: 24,
        indexCount: 36,
    };
}

export function createGridMesh(device: GPUDevice): Mesh {
    const gridSize = 10;
    const gridSpacing = 1.0;
    const vertices: number[] = [];
    
    for (let i = -gridSize; i <= gridSize; i++) {
        // Horizontal line
        vertices.push(i * gridSpacing, 0, -gridSize * gridSpacing);
        vertices.push(i * gridSpacing, 0, gridSize * gridSpacing);
        
        // Vertical line
        vertices.push(-gridSize * gridSpacing, 0, i * gridSpacing);
        vertices.push(gridSize * gridSpacing, 0, i * gridSpacing);
    }
    
    const vertexData = new Float32Array(vertices);
    
    const vertexBuffer = device.createBuffer({
        size: vertexData.byteLength,
        usage: GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST,
    });
    device.queue.writeBuffer(vertexBuffer, 0, vertexData);
    
    return {
        vertexBuffer,
        vertexCount: vertices.length / 3,
        indexCount: 0,
    };
}

export function createAxisMesh(device: GPUDevice): Mesh {
    // XYZ axes with colors
    const vertices = new Float32Array([
        // X axis (red)
        0, 0, 0, 1, 0, 0, 1, 0, 0,
        1, 0, 0, 1, 0, 0, 1, 0, 0,
        // Y axis (green)
        0, 0, 0, 0, 1, 0, 0, 1, 0,
        0, 1, 0, 0, 1, 0, 0, 1, 0,
        // Z axis (blue)
        0, 0, 0, 0, 0, 1, 0, 0, 1,
        0, 0, 1, 0, 0, 1, 0, 0, 1,
    ]);
    
    const vertexBuffer = device.createBuffer({
        size: vertices.byteLength,
        usage: GPUBufferUsage.VERTEX | GPUBufferUsage.COPY_DST,
    });
    device.queue.writeBuffer(vertexBuffer, 0, vertices);
    
    return {
        vertexBuffer,
        vertexCount: 6,
        indexCount: 0,
    };
}

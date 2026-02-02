// LUMA Viewer Web - Orbit Camera
// Maya-style orbit camera with spherical coordinates

import { mat4, vec3 } from './math';

export class OrbitCamera {
    // Spherical coordinates
    yaw = 0.78;      // 45 degrees
    pitch = 0.4;     // ~23 degrees
    distance = 3.0;
    
    // Target position
    targetX = 0;
    targetY = 0;
    targetZ = 0;
    
    // Projection params
    fovY = Math.PI / 4;  // 45 degrees
    aspectRatio = 1.0;
    nearZ = 0.01;
    farZ = 1000;
    
    orbit(deltaYaw: number, deltaPitch: number) {
        this.yaw += deltaYaw;
        this.pitch += deltaPitch;
        
        // Clamp pitch to avoid gimbal lock
        this.pitch = Math.max(-Math.PI / 2 + 0.1, Math.min(Math.PI / 2 - 0.1, this.pitch));
    }
    
    pan(deltaX: number, deltaY: number) {
        const cosYaw = Math.cos(this.yaw);
        const sinYaw = Math.sin(this.yaw);
        
        // Move in camera's local space
        this.targetX -= deltaX * this.distance * cosYaw;
        this.targetZ -= deltaX * this.distance * sinYaw;
        this.targetY += deltaY * this.distance;
    }
    
    zoom(factor: number) {
        this.distance *= factor;
        this.distance = Math.max(0.1, Math.min(100, this.distance));
    }
    
    reset() {
        this.yaw = 0.78;
        this.pitch = 0.4;
        this.distance = 3.0;
        this.targetX = 0;
        this.targetY = 0;
        this.targetZ = 0;
    }
    
    getPosition(): vec3 {
        const cosPitch = Math.cos(this.pitch);
        const sinPitch = Math.sin(this.pitch);
        const cosYaw = Math.cos(this.yaw);
        const sinYaw = Math.sin(this.yaw);
        
        return [
            this.targetX + this.distance * cosPitch * sinYaw,
            this.targetY + this.distance * sinPitch,
            this.targetZ + this.distance * cosPitch * cosYaw
        ];
    }
    
    getViewMatrix(): mat4 {
        const eye = this.getPosition();
        const target: vec3 = [this.targetX, this.targetY, this.targetZ];
        const up: vec3 = [0, 1, 0];
        
        return lookAt(eye, target, up);
    }
    
    getProjectionMatrix(): mat4 {
        return perspective(this.fovY, this.aspectRatio, this.nearZ, this.farZ);
    }
}

// Matrix helper functions
function lookAt(eye: vec3, target: vec3, up: vec3): mat4 {
    const f = normalize(subtract(target, eye));
    const s = normalize(cross(f, up));
    const u = cross(s, f);
    
    return [
        s[0], u[0], -f[0], 0,
        s[1], u[1], -f[1], 0,
        s[2], u[2], -f[2], 0,
        -dot(s, eye), -dot(u, eye), dot(f, eye), 1
    ];
}

function perspective(fovY: number, aspect: number, near: number, far: number): mat4 {
    const f = 1.0 / Math.tan(fovY / 2);
    const nf = 1 / (near - far);
    
    return [
        f / aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, (far + near) * nf, -1,
        0, 0, 2 * far * near * nf, 0
    ];
}

function subtract(a: vec3, b: vec3): vec3 {
    return [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
}

function normalize(v: vec3): vec3 {
    const len = Math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len === 0) return [0, 0, 0];
    return [v[0] / len, v[1] / len, v[2] / len];
}

function cross(a: vec3, b: vec3): vec3 {
    return [
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0]
    ];
}

function dot(a: vec3, b: vec3): number {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

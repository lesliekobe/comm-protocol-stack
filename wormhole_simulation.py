"""
虫洞可视化模拟
基于广义相对论引力透镜效应 + 空间隧道视觉效果
"""
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap
import matplotlib.animation as animation

# 虫洞参数
WORMHOLE_RADIUS = 0.4
SCHWARZSCHILD_RADIUS = 0.3

def gravitational_lensing(x, y, center_x=0, center_y=0):
    """计算引力透镜扭曲 - 虫洞周围空间弯曲效果"""
    r = np.sqrt((x - center_x)**2 + (y - center_y)**2)
    # 距离虫洞越近，空间扭曲越强（类似爱因斯坦环）
    distortion = 1 / (1 + (r / SCHWARZSCHILD_RADIUS)**2)
    return distortion

def wormhole_metric(r):
    """
    简化的虫洞度规（Ellis wormhole）
    ds² = -dt² + dl² + r²(dθ² + sin²θ dφ²)
    其中 r 是虫洞半径函数
    """
    # 虫洞喉部半径
    return np.sqrt(r**2 + 0.1)

def create_tunnel_effect(size=200):
    """创建虫洞隧道视觉效果"""
    tunnel = np.zeros((size, size, 3))
    center = size // 2
    
    for i in range(size):
        for j in range(size):
            # 到中心距离
            r = np.sqrt((i - center)**2 + (j - center)**2)
            max_r = size // 2
            
            # 归一化
            norm_r = r / max_r
            
            # 虫洞颜色渐变：从边缘的深红/橙到中心的蓝白（高能量）
            if norm_r < WORMHOLE_RADIUS:
                # 虫洞内部 - 高能蓝光
                t = norm_r / WORMHOLE_RADIUS
                tunnel[i, j] = [
                    0.2 + 0.8 * t,  # R
                    0.4 + 0.6 * t,  # G
                    1.0              # B
                ]
            else:
                # 虫洞外部 - 空间扭曲效果
                distortion = gravitational_lensing(i - center, j - center)
                # 橙红色吸积盘
                if distortion > 0.3:
                    intensity = (distortion - 0.3) * 2
                    tunnel[i, j] = [
                        intensity * 1.0,
                        intensity * 0.3,
                        intensity * 0.1
                    ]
                else:
                    # 背景星空
                    tunnel[i, j] = [0.02, 0.02, 0.08]
    
    return tunnel

def draw_einstein_ring(ax, t):
    """绘制动态的爱因斯坦环效果"""
    theta = np.linspace(0, 2 * np.pi, 100)
    # 环半径随时间脉动（模拟能量波动）
    r_ring = 0.5 + 0.05 * np.sin(t * 3)
    
    x = r_ring * np.cos(theta)
    y = r_ring * np.sin(theta)
    
    # 环的光度变化
    alpha = 0.5 + 0.3 * np.sin(t * 5)
    ax.plot(x, y, color='orange', linewidth=2, alpha=alpha)

def animate_wormhole():
    """虫洞动画 - 展示能量脉动和引力透镜"""
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    
    frames = 100
    
    def update(frame):
        t = frame / frames * 2 * np.pi
        
        # 左图：引力透镜静态图
        ax1 = axes[0]
        ax1.clear()
        ax1.set_xlim(-1.5, 1.5)
        ax1.set_ylim(-1.5, 1.5)
        ax1.set_aspect('equal')
        ax1.axis('off')
        ax1.set_title('引力透镜效应 (Gravitational Lensing)', fontsize=12)
        
        # 绘制背景星空
        np.random.seed(42)
        stars_x = np.random.uniform(-1.5, 1.5, 200)
        stars_y = np.random.uniform(-1.5, 1.5, 200)
        ax1.scatter(stars_x, stars_y, s=1, c='white', alpha=0.6)
        
        # 绘制扭曲的光线（简化的引力透镜）
        for angle in np.linspace(0, 2*np.pi, 8):
            x_line = np.linspace(-1.4, 1.4, 50)
            r = np.sqrt(x_line**2 + 0.01)
            y_line = 0.3 * np.sin(angle) / (1 + r**2 * 3)
            ax1.plot(x_line, y_line, 'y-', alpha=0.1, linewidth=0.5)
        
        # 爱因斯坦环
        theta = np.linspace(0, 2*np.pi, 100)
        r_einstein = 0.5 + 0.02 * np.sin(t * 2)
        x_ring = r_einstein * np.cos(theta)
        y_ring = r_einstein * np.sin(theta)
        ax1.plot(x_ring, y_ring, color='#ff6600', linewidth=2.5, alpha=0.8)
        
        # 中心虫洞
        circle = plt.Circle((0, 0), 0.15, color='cyan', alpha=0.9)
        ax1.add_patch(circle)
        
        # 右图：虫洞隧道
        ax2 = axes[1]
        ax2.clear()
        ax2.set_xlim(-1, 1)
        ax2.set_ylim(-1, 1)
        ax2.axis('off')
        ax2.set_title('虫洞隧道 (Wormhole Tunnel)', fontsize=12)
        
        tunnel = create_tunnel_effect(200)
        # 动态缩放效果
        scale = 1 + 0.1 * np.sin(t * 4)
        ax2.imshow(tunnel, extent=[-scale, scale, -scale, scale], origin='lower')
        
        # 添加旋转效果
        rotation = t * 0.5
        for i in range(6):
            angle = rotation + i * np.pi / 3
            x = 0.3 * np.cos(angle)
            y = 0.3 * np.sin(angle)
            ax2.annotate('', xy=(x*0.8, y*0.8), xytext=(x*1.2, y*1.2),
                        arrowprops=dict(arrowstyle='->', color='white', lw=1.5, alpha=0.5))
    
    anim = animation.FuncAnimation(fig, update, frames=frames, interval=50, repeat=True)
    
    plt.tight_layout()
    plt.savefig('wormhole_simulation.png', dpi=150, bbox_inches='tight', facecolor='black')
    print("✅ 虫洞模拟图已保存: wormhole_simulation.png")
    
    # 同时生成静态高清图
    fig2, ax = plt.subplots(figsize=(10, 10), facecolor='black')
    ax.set_facecolor('black')
    ax.set_xlim(-1.5, 1.5)
    ax.set_ylim(-1.5, 1.5)
    ax.axis('off')
    ax.set_title('Wormhole Visualization\n虫洞可视化', fontsize=16, color='white', pad=20)
    
    # 星空背景
    np.random.seed(42)
    stars_x = np.random.uniform(-1.5, 1.5, 300)
    stars_y = np.random.uniform(-1.5, 1.5, 300)
    ax.scatter(stars_x, stars_y, s=1.5, c='white', alpha=0.5)
    
    # 引力透镜扭曲线
    for angle in np.linspace(0, 2*np.pi, 12):
        x_line = np.linspace(-1.4, 1.4, 50)
        r = np.sqrt(x_line**2 + 0.01)
        y_line = 0.4 * np.sin(angle) / (1 + r**2 * 3)
        ax.plot(x_line, y_line, 'y-', alpha=0.15, linewidth=0.5)
    
    # 主爱因斯坦环
    theta = np.linspace(0, 2*np.pi, 100)
    x_ring = 0.55 * np.cos(theta)
    y_ring = 0.55 * np.sin(theta)
    ax.plot(x_ring, y_ring, color='#ff8800', linewidth=3, alpha=0.9)
    
    # 内环
    x_ring2 = 0.35 * np.cos(theta)
    y_ring2 = 0.35 * np.sin(theta)
    ax.plot(x_ring2, y_ring2, color='#ffaa44', linewidth=2, alpha=0.6)
    
    # 虫洞核心
    core = plt.Circle((0, 0), 0.18, color='#00ffff', alpha=0.95)
    ax.add_patch(core)
    core_inner = plt.Circle((0, 0), 0.08, color='white', alpha=0.9)
    ax.add_patch(core_inner)
    
    # 说明文字
    ax.text(0, -1.3, '虫洞半径 ~0.18 | 爱因斯坦环半径 ~0.55', 
            ha='center', fontsize=10, color='gray')
    
    plt.savefig('wormhole_static.png', dpi=200, bbox_inches='tight', facecolor='black')
    print("✅ 静态高清图已保存: wormhole_static.png")
    
    plt.close('all')

if __name__ == "__main__":
    animate_wormhole()
    print("🎉 虫洞模拟完成!")
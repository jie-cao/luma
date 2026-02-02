// LUMA Viewer iOS - Main View Controller
// Metal-based 3D model viewer with touch gesture controls

#import <UIKit/UIKit.h>
#import <MetalKit/MetalKit.h>

@interface LumaViewController : UIViewController

/// Load a 3D model from file path
- (void)loadModel:(NSString *)path;

/// Load a LUMA scene file
- (void)loadScene:(NSString *)path;

/// Reset camera to default view
- (void)resetCamera;

/// Set auto-rotation enabled
@property (nonatomic, assign) BOOL autoRotate;

/// Set wireframe mode
@property (nonatomic, assign) BOOL wireframeMode;

/// Set show grid
@property (nonatomic, assign) BOOL showGrid;

@end

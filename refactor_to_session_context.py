"""
Refactor VisualizationApi.cpp to use per-session context instead of global variables.

Key changes:
1. Create a comprehensive TabSessionContext that holds all per-tab data
2. Move global variables into TabSessionContext
3. Use sessionId to look up the context for all operations

This script will:
1. Read the current file
2. Identify and move global variables to session context
3. Create helper functions to get session context
"""

import re
import shutil

src = r'D:\2025-09-25 新系列\ConsoleDllTest\DllVisualization\VisualizationApi.cpp'
bak = src + '.before_session_refactor.bak'

# Create backup
shutil.copy2(src, bak)
print(f'Created backup: {bak}')

with open(src, 'rb') as f:
    content = f.read().decode('utf-8', errors='replace')

original_size = len(content)
print(f'Original file size: {original_size} chars')

# Define the new TabSessionContext struct
new_session_context = '''
// ==================== Per-Tab Session Context ====================
// All data that belongs to a single tab/session should be stored here.
// This ensures complete isolation between tabs and avoids global state conflicts.

struct TabSessionContext {
    std::string sessionId;
    
    // Volume data
    VolumeHandle volumeHandle = nullptr;
    MPRHandle mprHandle = nullptr;
    
    // Cropped volume (for crop operations)
    APRHandle croppedAPR = nullptr;
    
    // Crop box state
    GlobalAPRCropBox cropBox;
    
    // 3D view state
    float rotX = 30.0f;
    float rotY = 45.0f;
    float zoom = 1.0f;
    float panX = 0.0f;
    float panY = 0.0f;
    float rotMat[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    bool rotating = false;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    
    // APR center (shared by linked APRs in this tab)
    GlobalAPRCenter aprCenter;
    
    // MPR center
    GlobalMPRCenter mprCenter;
    
    // All APR handles for this session
    std::vector<APRHandle> linkedAPRs;
    
    // Measurement state
    std::vector<CompletedMeasurement> completedMeasurements;
    int nextMeasurementId = 1;
    int hoverMeasurementIndex = -1;
    int hoverPointIndex = -1;
    int dragMeasurementIndex = -1;
    int dragPointIndex = -1;
    bool isDraggingPoint = false;
    std::vector<MeasurementPoint> measurementPoints;
    MeasurementPoint currentMousePos = {0.0f, 0.0f, 0.0f};
    MeasurementLocation currentMeasurementLocation;
    bool isDrawing = false;
    int currentToolType = 0;
    int lastMeasurementTool = 1;
    bool shiftPressed = false;
    double lastClickTime = 0.0;
    
    // Mask editing state
    int currentMaskIndex = -1;
    float brushRadius = 5.0f;
    void* currentMaskManager = nullptr;
    int currentMaskTool = 1;
    std::string maskEditSessionId;
    int maskEditMaskId = -1;
    std::vector<Point2D> maskStrokePath;
    bool maskStrokeNeedsUpdate = false;
    
    // Windows belonging to this tab
    std::vector<WindowHandle> windows;
    
    // 3D primitives
    std::vector<Primitive3D> primitives;
    int nextPrimitiveId = 1;
    Scene3DTransform sceneTransform;
    
    // Constructor
    TabSessionContext() {
        Mat4_Identity(rotMat);
    }
    
    // Destructor - clean up resources
    ~TabSessionContext() {
        // Clean up APRs
        for (auto apr : linkedAPRs) {
            if (apr) {
                // Note: actual cleanup should be done before destruction
            }
        }
        if (croppedAPR) {
            // Clean up cropped APR
        }
    }
};

// Global map of all tab sessions
static std::map<std::string, std::unique_ptr<TabSessionContext>> g_TabSessions;
static std::mutex g_TabSessionMutex;

// Helper to get or create a tab session context
static TabSessionContext* GetTabSession(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(g_TabSessionMutex);
    auto it = g_TabSessions.find(sessionId);
    if (it != g_TabSessions.end()) {
        return it->second.get();
    }
    // Create new session
    auto ctx = std::make_unique<TabSessionContext>();
    ctx->sessionId = sessionId;
    TabSessionContext* ptr = ctx.get();
    g_TabSessions[sessionId] = std::move(ctx);
    return ptr;
}

// Helper to get existing tab session (returns nullptr if not found)
static TabSessionContext* FindTabSession(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(g_TabSessionMutex);
    auto it = g_TabSessions.find(sessionId);
    if (it != g_TabSessions.end()) {
        return it->second.get();
    }
    return nullptr;
}

// Destroy a tab session and all its resources
static void DestroyTabSession(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(g_TabSessionMutex);
    auto it = g_TabSessions.find(sessionId);
    if (it != g_TabSessions.end()) {
        g_TabSessions.erase(it);
    }
}

'''

# For now, let's just add the new structure after the existing SessionContext
# and mark the old globals as deprecated with TODO comments

# Find the location after SessionContext definition
old_session_pattern = r'(static std::map<std::string, SessionContext> g_Sessions;)'
match = re.search(old_session_pattern, content)

if match:
    insert_pos = match.end()
    # Insert the new TabSessionContext after g_Sessions
    content = content[:insert_pos] + '\n' + new_session_context + content[insert_pos:]
    print('1. Added TabSessionContext structure')
else:
    print('1. WARNING: Could not find g_Sessions declaration')

# Add deprecation comments to old globals
deprecated_globals = [
    ('static GlobalAPRCropBox g_aprCropBox;', 
     '// DEPRECATED: Use TabSessionContext::cropBox instead\nstatic GlobalAPRCropBox g_aprCropBox;'),
    ('static APRHandle g_lastCroppedAPR = nullptr;',
     '// DEPRECATED: Use TabSessionContext::croppedAPR instead\nstatic APRHandle g_lastCroppedAPR = nullptr;'),
    ('static float g_3dRotX = 30.0f;',
     '// DEPRECATED: Use TabSessionContext::rotX instead\nstatic float g_3dRotX = 30.0f;'),
    ('static GlobalMPRCenter g_globalMPRCenter;',
     '// DEPRECATED: Use TabSessionContext::mprCenter instead\nstatic GlobalMPRCenter g_globalMPRCenter;'),
    ('static GlobalAPRCenter g_globalAPRCenter;',
     '// DEPRECATED: Use TabSessionContext::aprCenter instead\nstatic GlobalAPRCenter g_globalAPRCenter;'),
]

for old, new in deprecated_globals:
    if old in content:
        content = content.replace(old, new, 1)
        print(f'   Marked as deprecated: {old[:50]}...')

# Write result
with open(src, 'wb') as f:
    f.write(content.encode('utf-8'))

final_size = len(content)
print(f'\nFinal file size: {final_size} chars (delta: {final_size - original_size})')
print('\nDone! The TabSessionContext structure has been added.')
print('Next steps:')
print('  1. Gradually migrate functions to use TabSessionContext instead of global variables')
print('  2. Update APR_Create, MPR_Create etc. to register with TabSessionContext')
print('  3. Update all functions that use global state to accept sessionId parameter')

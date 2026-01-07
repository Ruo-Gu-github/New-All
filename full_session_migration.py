"""
Complete migration of all global variables to TabSessionContext.

This script will:
1. Update TabSessionContext to include ALL per-session state
2. Create helper functions to access session state
3. Migrate key functions to use TabSessionContext
"""

import re
import shutil

src = r'D:\2025-09-25 新系列\ConsoleDllTest\DllVisualization\VisualizationApi.cpp'
bak = src + '.full_migration.bak'

# Create backup
shutil.copy2(src, bak)
print(f'Created backup: {bak}')

with open(src, 'rb') as f:
    content = f.read().decode('utf-8', errors='replace')

original_size = len(content)
print(f'Original file size: {original_size} chars')

# Find and replace the TabSessionContext struct with a complete version
old_struct_start = '// ==================== Per-Tab Session Context ===================='
old_struct_end = '// Helper to get existing tab session (returns nullptr if not found)'

if old_struct_start in content and old_struct_end in content:
    start_idx = content.find(old_struct_start)
    end_idx = content.find(old_struct_end)
    
    new_struct = '''// ==================== Per-Tab Session Context ====================
// All data that belongs to a single tab/session should be stored here.
// This ensures complete isolation between tabs and avoids global state conflicts.

struct TabSessionContext {
    std::string sessionId;
    
    // ==================== Volume and Rendering ====================
    VolumeHandle volumeHandle = nullptr;
    MPRHandle mprHandle = nullptr;
    
    // All APR handles for this session (multiple views sharing same volume)
    std::vector<APRHandle> linkedAPRs;
    
    // Cropped volume result
    APRHandle croppedAPR = nullptr;
    
    // ==================== Crop Box State ====================
    GlobalAPRCropBox cropBox;
    
    // ==================== 3D View State ====================
    float rotX = 30.0f;
    float rotY = 45.0f;
    float zoom = 1.0f;
    float panX = 0.0f;
    float panY = 0.0f;
    float rotMat[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    bool rotating = false;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    
    // ==================== APR/MPR Center (shared by linked views) ====================
    GlobalAPRCenter aprCenter;
    GlobalMPRCenter mprCenter;
    
    // ==================== Measurement State ====================
    std::vector<CompletedMeasurement> completedMeasurements;
    std::mutex measurementsMutex;
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
    int currentToolType = 0;  // 0=navigate, 1+=measurement tools
    int lastMeasurementTool = 1;
    bool shiftPressed = false;
    double lastClickTime = 0.0;
    
    // ==================== Mask Editing State ====================
    int currentMaskIndex = -1;
    float brushRadius = 5.0f;
    void* currentMaskManager = nullptr;
    int currentMaskTool = 1;  // 1=brush, 2=eraser, etc.
    std::string maskEditSessionId;
    int maskEditMaskId = -1;
    std::vector<Point2D> maskStrokePath;
    bool maskStrokeNeedsUpdate = false;
    
    // ==================== Windows belonging to this tab ====================
    std::vector<WindowHandle> windows;
    
    // ==================== 3D Primitives ====================
    std::vector<Primitive3D> primitives;
    int nextPrimitiveId = 1;
    Scene3DTransform sceneTransform;
    
    // ==================== Constructor/Destructor ====================
    TabSessionContext() {
        // Initialize rotation matrix to identity
        rotMat[0] = rotMat[5] = rotMat[10] = rotMat[15] = 1.0f;
        rotMat[1] = rotMat[2] = rotMat[3] = rotMat[4] = 0.0f;
        rotMat[6] = rotMat[7] = rotMat[8] = rotMat[9] = 0.0f;
        rotMat[11] = rotMat[12] = rotMat[13] = rotMat[14] = 0.0f;
    }
    
    ~TabSessionContext() {
        // Resources should be cleaned up via Session_Destroy before destruction
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

'''
    
    content = content[:start_idx] + new_struct + content[end_idx:]
    print('1. Updated TabSessionContext struct with complete state')

# Now let's update key functions to use TabSessionContext
# We'll add helper macros/functions at the top

# Find location after TabSessionContext helpers
helper_insert_marker = '// Destroy a tab session and all its resources\nstatic void DestroyTabSession'
if helper_insert_marker in content:
    idx = content.find(helper_insert_marker)
    
    session_helpers = '''
// ==================== Session State Access Helpers ====================
// These helpers provide easy access to session-specific state
// They first try TabSessionContext, then fall back to global variables

// Get measurement state for a session
static std::vector<CompletedMeasurement>* GetSessionMeasurements(const std::string& sessionId) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) return &ctx->completedMeasurements;
    return &g_completedMeasurements; // fallback to global
}

// Get current tool type for a session
static int GetSessionToolType(const std::string& sessionId) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) return ctx->currentToolType;
    return g_currentToolType;
}

static void SetSessionToolType(const std::string& sessionId, int toolType) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) ctx->currentToolType = toolType;
    g_currentToolType = toolType; // also update global for compatibility
}

// Get crop box for a session
static GlobalAPRCropBox* GetSessionCropBox(const std::string& sessionId) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) return &ctx->cropBox;
    return &g_aprCropBox;
}

// Get APR center for a session
static GlobalAPRCenter* GetSessionAPRCenter(const std::string& sessionId) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) return &ctx->aprCenter;
    return &g_globalAPRCenter;
}

// Get 3D view state for a session
static void GetSession3DViewState(const std::string& sessionId, 
                                   float* rotX, float* rotY, float* zoom,
                                   float* panX, float* panY, float** rotMat) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) {
        if (rotX) *rotX = ctx->rotX;
        if (rotY) *rotY = ctx->rotY;
        if (zoom) *zoom = ctx->zoom;
        if (panX) *panX = ctx->panX;
        if (panY) *panY = ctx->panY;
        if (rotMat) *rotMat = ctx->rotMat;
    } else {
        if (rotX) *rotX = g_3dRotX;
        if (rotY) *rotY = g_3dRotY;
        if (zoom) *zoom = g_3dZoom;
        if (panX) *panX = g_3dPanX;
        if (panY) *panY = g_3dPanY;
        if (rotMat) *rotMat = g_3dRotMat;
    }
}

static void SetSession3DViewState(const std::string& sessionId,
                                   float rotX, float rotY, float zoom,
                                   float panX, float panY) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) {
        ctx->rotX = rotX;
        ctx->rotY = rotY;
        ctx->zoom = zoom;
        ctx->panX = panX;
        ctx->panY = panY;
    }
    // Also update globals for compatibility
    g_3dRotX = rotX;
    g_3dRotY = rotY;
    g_3dZoom = zoom;
    g_3dPanX = panX;
    g_3dPanY = panY;
}

// Get mask editing state for a session
static void GetSessionMaskState(const std::string& sessionId,
                                 int* maskIndex, float* brushRadius, int* maskTool) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) {
        if (maskIndex) *maskIndex = ctx->currentMaskIndex;
        if (brushRadius) *brushRadius = ctx->brushRadius;
        if (maskTool) *maskTool = ctx->currentMaskTool;
    } else {
        if (maskIndex) *maskIndex = g_currentMaskIndex;
        if (brushRadius) *brushRadius = g_brushRadius;
        if (maskTool) *maskTool = g_currentMaskTool;
    }
}

static void SetSessionMaskState(const std::string& sessionId,
                                 int maskIndex, float brushRadius, int maskTool) {
    TabSessionContext* ctx = FindTabSession(sessionId);
    if (ctx) {
        ctx->currentMaskIndex = maskIndex;
        ctx->brushRadius = brushRadius;
        ctx->currentMaskTool = maskTool;
    }
    g_currentMaskIndex = maskIndex;
    g_brushRadius = brushRadius;
    g_currentMaskTool = maskTool;
}

'''
    content = content[:idx] + session_helpers + content[idx:]
    print('2. Added session state access helper functions')

# Write result
with open(src, 'wb') as f:
    f.write(content.encode('utf-8'))

final_size = len(content)
print(f'\nFinal file size: {final_size} chars (delta: {final_size - original_size})')
print('\nDone! Phase 1 complete.')
print('The TabSessionContext now contains all per-session state.')
print('Helper functions have been added for easy access.')

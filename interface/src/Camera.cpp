//
//  Camera.cpp
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <glm/gtx/quaternion.hpp>

#include <SharedUtil.h>
#include <VoxelConstants.h>
#include <EventTypes.h>

#include "Application.h"
#include "Camera.h"
#include "Menu.h"
#include "Util.h"

const float CAMERA_MINIMUM_MODE_SHIFT_RATE     = 0.5f;

const float CAMERA_FIRST_PERSON_MODE_UP_SHIFT  = 0.0f;
const float CAMERA_FIRST_PERSON_MODE_DISTANCE  = 0.0f;
const float CAMERA_FIRST_PERSON_MODE_TIGHTNESS = 100.0f;

const float CAMERA_INDEPENDENT_MODE_UP_SHIFT  = 0.0f;
const float CAMERA_INDEPENDENT_MODE_DISTANCE  = 0.0f;
const float CAMERA_INDEPENDENT_MODE_TIGHTNESS = 100.0f;

const float CAMERA_THIRD_PERSON_MODE_UP_SHIFT  = -0.2f;
const float CAMERA_THIRD_PERSON_MODE_DISTANCE  = 1.5f;
const float CAMERA_THIRD_PERSON_MODE_TIGHTNESS = 8.0f;

const float CAMERA_MIRROR_MODE_UP_SHIFT        = 0.0f;
const float CAMERA_MIRROR_MODE_DISTANCE        = 0.17f;
const float CAMERA_MIRROR_MODE_TIGHTNESS       = 100.0f;


Camera::Camera() : 
    _needsToInitialize(true),
    _mode(CAMERA_MODE_THIRD_PERSON),
    _prevMode(CAMERA_MODE_THIRD_PERSON),
    _frustumNeedsReshape(true),
    _position(0.0f, 0.0f, 0.0f),
    _idealPosition(0.0f, 0.0f, 0.0f),
    _targetPosition(0.0f, 0.0f, 0.0f),
    _fieldOfView(DEFAULT_FIELD_OF_VIEW_DEGREES),
    _aspectRatio(16.f/9.f),
    _nearClip(0.08f), // default
    _farClip(50.0f * TREE_SCALE), // default
    _upShift(0.0f),
    _distance(0.0f),
    _tightness(10.0f), // default
    _previousUpShift(0.0f),
    _previousDistance(0.0f),
    _previousTightness(0.0f),
    _newUpShift(0.0f),
    _newDistance(0.0f),
    _newTightness(0.0f),
    _modeShift(1.0f),
    _linearModeShift(0.0f),
    _modeShiftRate(1.0f),
    _scale(1.0f)
{
}

void Camera::update(float deltaTime)  {

    if (_mode != CAMERA_MODE_NULL) {
        // use iterative forces to push the camera towards the target position and angle
        updateFollowMode(deltaTime);
     }
}

// use iterative forces to keep the camera at the desired position and angle
void Camera::updateFollowMode(float deltaTime) {  
    if (_linearModeShift < 1.0f) {
        _linearModeShift += _modeShiftRate * deltaTime;
        _modeShift = ONE_HALF - ONE_HALF * cosf(_linearModeShift * PIE );
        _upShift   = _previousUpShift   * (1.0f - _modeShift) + _newUpShift   * _modeShift;
        _distance  = _previousDistance  * (1.0f - _modeShift) + _newDistance  * _modeShift;
        _tightness = _previousTightness * (1.0f - _modeShift) + _newTightness * _modeShift;

        if (_needsToInitialize || _linearModeShift > 1.0f) {
            _linearModeShift = 1.0f;
            _modeShift = 1.0f;
            _upShift   = _newUpShift;
            _distance  = _newDistance;
            _tightness = _newTightness;
        }
    }

    // derive t from tightness
    float t = _tightness * _modeShift * deltaTime;	
    if (t > 1.0) {
        t = 1.0;
    }
    
    // Update position and rotation, setting directly if tightness is 0.0
    
    if (_needsToInitialize || (_tightness == 0.0f)) {
        _rotation = _targetRotation;
        _idealPosition = _targetPosition + _scale * (_rotation * glm::vec3(0.0f, _upShift, _distance));
        _position = _idealPosition;
        _needsToInitialize = false;
    } else {
        // pull rotation towards ideal
        _rotation = safeMix(_rotation, _targetRotation, t);
        _idealPosition = _targetPosition + _scale * (_rotation * glm::vec3(0.0f, _upShift, _distance));
        _position += (_idealPosition - _position) * t;
    }
}

float Camera::getFarClip() const {
    return (_scale * _farClip < std::numeric_limits<int16_t>::max())
            ? _scale * _farClip
            : std::numeric_limits<int16_t>::max() - 1;
}

void Camera::setModeShiftRate ( float rate ) {
    
    _modeShiftRate = rate;
    
    if (_modeShiftRate < CAMERA_MINIMUM_MODE_SHIFT_RATE ) {
        _modeShiftRate = CAMERA_MINIMUM_MODE_SHIFT_RATE;
    }
}

void Camera::setMode(CameraMode m) { 
 
    _prevMode = _mode;
    _mode = m;
    _modeShift = 0.0;
    _linearModeShift = 0.0;
    
    _previousUpShift   = _upShift;
    _previousDistance  = _distance;
    _previousTightness = _tightness;

    if (_mode == CAMERA_MODE_THIRD_PERSON) {
        _newUpShift   = CAMERA_THIRD_PERSON_MODE_UP_SHIFT;
        _newDistance  = CAMERA_THIRD_PERSON_MODE_DISTANCE;
        _newTightness = CAMERA_THIRD_PERSON_MODE_TIGHTNESS;
    } else if (_mode == CAMERA_MODE_FIRST_PERSON) {
        _newUpShift   = CAMERA_FIRST_PERSON_MODE_UP_SHIFT;
        _newDistance  = CAMERA_FIRST_PERSON_MODE_DISTANCE;
        _newTightness = CAMERA_FIRST_PERSON_MODE_TIGHTNESS;
    } else if (_mode == CAMERA_MODE_MIRROR) {
        _newUpShift   = CAMERA_MIRROR_MODE_UP_SHIFT;
        _newDistance  = CAMERA_MIRROR_MODE_DISTANCE;
        _newTightness = CAMERA_MIRROR_MODE_TIGHTNESS;
    } else if (_mode == CAMERA_MODE_INDEPENDENT) {
        _newUpShift   = CAMERA_INDEPENDENT_MODE_UP_SHIFT;
        _newDistance  = CAMERA_INDEPENDENT_MODE_DISTANCE;
        _newTightness = CAMERA_INDEPENDENT_MODE_TIGHTNESS;
        
    }
}


void Camera::setTargetRotation( const glm::quat& targetRotation ) {
    _targetRotation = targetRotation;
}

void Camera::setFieldOfView(float f) { 
    _fieldOfView = f; 
    _frustumNeedsReshape = true; 
}

void Camera::setAspectRatio(float a) { 
    _aspectRatio = a; 
    _frustumNeedsReshape = true; 
}

void Camera::setNearClip(float n) { 
    _nearClip = n; 
    _frustumNeedsReshape = true; 
}

void Camera::setFarClip(float f) { 
    _farClip = f; 
    _frustumNeedsReshape = true; 
}

void Camera::setEyeOffsetPosition(const glm::vec3& p) {
    _eyeOffsetPosition = p;
    _frustumNeedsReshape = true;
}

void Camera::setEyeOffsetOrientation(const glm::quat& o) {
    _eyeOffsetOrientation = o;
    _frustumNeedsReshape = true;
}

void Camera::setScale(float s) {
    _scale = s;
    _needsToInitialize = true;
    _frustumNeedsReshape = true;
}

void Camera::initialize() {
    _needsToInitialize = true;
    _modeShift = 0.0;
}

// call to find out if the view frustum needs to be reshaped
bool Camera::getFrustumNeedsReshape() const {
    return _frustumNeedsReshape;
}

// call this when deciding whether to render the head or not
CameraMode Camera::getInterpolatedMode() const {
    const float SHIFT_THRESHOLD_INTO_FIRST_PERSON = 0.7f;
    const float SHIFT_THRESHOLD_OUT_OF_FIRST_PERSON = 0.6f;
    if ((_mode == CAMERA_MODE_FIRST_PERSON && _linearModeShift < SHIFT_THRESHOLD_INTO_FIRST_PERSON) ||
        (_prevMode == CAMERA_MODE_FIRST_PERSON && _linearModeShift < SHIFT_THRESHOLD_OUT_OF_FIRST_PERSON)) {
        return _prevMode;
    }
    return _mode;
}

// call this after reshaping the view frustum
void Camera::setFrustumWasReshaped() {
    _frustumNeedsReshape = false;
}


CameraScriptableObject::CameraScriptableObject(Camera* camera, ViewFrustum* viewFrustum) :
    _camera(camera), _viewFrustum(viewFrustum) 
{
}

PickRay CameraScriptableObject::computePickRay(float x, float y) {
    float screenWidth = Application::getInstance()->getGLWidget()->width();
    float screenHeight = Application::getInstance()->getGLWidget()->height();
    PickRay result;
    _viewFrustum->computePickRay(x / screenWidth, y / screenHeight, result.origin, result.direction);
    return result;
}

QString CameraScriptableObject::getMode() const {
    QString mode("unknown");
    switch(_camera->getMode()) {
        case CAMERA_MODE_THIRD_PERSON:
            mode = "third person";
            break;
        case CAMERA_MODE_FIRST_PERSON:
            mode = "first person";
            break;
        case CAMERA_MODE_MIRROR:
            mode = "mirror";
            break;
        case CAMERA_MODE_INDEPENDENT:
            mode = "independent";
            break;
        default:
            break;
    }
    return mode;
}

void CameraScriptableObject::setMode(const QString& mode) {
    CameraMode currentMode = _camera->getMode();
    CameraMode targetMode = currentMode;
    if (mode == "third person") {
        targetMode = CAMERA_MODE_THIRD_PERSON;
        Menu::getInstance()->setIsOptionChecked(MenuOption::FullscreenMirror, false);
        Menu::getInstance()->setIsOptionChecked(MenuOption::FirstPerson, false);
    } else if (mode == "first person") {
        targetMode = CAMERA_MODE_FIRST_PERSON;
        Menu::getInstance()->setIsOptionChecked(MenuOption::FullscreenMirror, false);
        Menu::getInstance()->setIsOptionChecked(MenuOption::FirstPerson, true);
    } else if (mode == "mirror") {
        targetMode = CAMERA_MODE_MIRROR;
        Menu::getInstance()->setIsOptionChecked(MenuOption::FullscreenMirror, true);
        Menu::getInstance()->setIsOptionChecked(MenuOption::FirstPerson, false);
    } else if (mode == "independent") {
        targetMode = CAMERA_MODE_INDEPENDENT;
        Menu::getInstance()->setIsOptionChecked(MenuOption::FullscreenMirror, false);
        Menu::getInstance()->setIsOptionChecked(MenuOption::FirstPerson, false);
    }
    if (currentMode != targetMode) {
        _camera->setMode(targetMode);
        _camera->setModeShiftRate(10.0f);
    }
}




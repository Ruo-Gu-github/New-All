const path = require('node:path');
const fs = require('node:fs');

function resolveBinary() {
  const releasePath = path.join(__dirname, 'build', 'Release', 'dicom_native.node');
  if (fs.existsSync(releasePath)) {
    return releasePath;
  }
  const debugPath = path.join(__dirname, 'build', 'Debug', 'dicom_native.node');
  if (fs.existsSync(debugPath)) {
    return debugPath;
  }
  throw new Error('dicom_native.node 尚未构建，请先执行 "npm run build:native:dicom"。');
}

module.exports = require(resolveBinary());

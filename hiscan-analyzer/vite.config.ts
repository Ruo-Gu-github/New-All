import { defineConfig } from "vite";
import path from "node:path";
import electron from "vite-plugin-electron/simple";
import vue from "@vitejs/plugin-vue";

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [
    vue(),
    electron({
      main: {
        // Shortcut of `build.lib.entry`.
        entry: "electron/main.ts",
      },
      preload: {
        // Shortcut of `build.rollupOptions.input`.
        // Preload scripts may contain Web assets, so use the `build.rollupOptions.input` instead `build.lib.entry`.
        input: path.join(__dirname, "electron/preload.ts"),
      },
      // Ployfill the Electron and Node.js API for Renderer process.
      // If you want use Node.js in Renderer process, the `nodeIntegration` needs to be enabled in the Main process.
      // See 👉 https://github.com/electron-vite/vite-plugin-electron-renderer
      renderer:
        process.env.NODE_ENV === "test"
          ? // https://github.com/electron-vite/vite-plugin-electron-renderer/issues/78#issuecomment-2053600808
            undefined
          : {},
    }),
  ],
  build: {
    rollupOptions: {
      input: {
        main: path.resolve(__dirname, "index.html"),
        dialog: path.resolve(__dirname, "dialog.html"),
        dialogRigmark: path.resolve(__dirname, "dialog-rigmark.html"),
        dialogMorphology: path.resolve(__dirname, "dialog-morphology.html"),
        dialogBoolean: path.resolve(__dirname, "dialog-boolean.html"),
        dialogCropsettings: path.resolve(__dirname, "dialog-cropsettings.html"),
        dialogConfig3d: path.resolve(__dirname, "dialog-config3d.html"),
        dialogStats: path.resolve(__dirname, "dialog-stats.html"),
        dialogColorpicker: path.resolve(__dirname, "dialog-colorpicker.html"),
        dialogOpacity: path.resolve(__dirname, "dialog-opacity.html"),
        dialogRoiedit: path.resolve(__dirname, "dialog-roiedit.html"),
        dialogSavemask: path.resolve(__dirname, "dialog-savemask.html"),
        dialogScreenshot: path.resolve(__dirname, "dialog-screenshot.html"),
        dialogMeasurements: path.resolve(__dirname, "dialog-measurements.html"),
        dialogMeasurementChart: path.resolve(
          __dirname,
          "dialog-measurement-chart.html"
        ),
        dialogLighting: path.resolve(__dirname, "dialog-lighting.html"),
        dialogTransferfunction: path.resolve(
          __dirname,
          "dialog-transferfunction.html"
        ),
      },
    },
  },
});

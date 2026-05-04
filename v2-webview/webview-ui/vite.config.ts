import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import { viteSingleFile } from "vite-plugin-singlefile";

// Single-file inlined HTML so we only have to embed ONE file as JUCE BinaryData.
// vite-plugin-singlefile handles the rollup config (sets inlineDynamicImports,
// disables code-splitting, inlines all assets).
export default defineConfig({
  plugins: [react(), viteSingleFile()],
  build: {
    target: "es2020"
  }
});

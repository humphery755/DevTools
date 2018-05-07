/* eslint strict: 0 */
'use strict';

const path = require('path');
const webpack = require('webpack');

module.exports ={
  target: 'electron-renderer',
  entry: {
      home: "./renderer-process/renderer.js"
  },
  output: {
    path: path.join(__dirname, 'build'),
    publicPath: path.join(__dirname, 'renderer-process'),
    filename: '[name].bundle.js',
  },
  module: {
    rules: [{
      test: /\.css$/,
      use: [ 'style-loader', 'css-loader' ]
    }]
  }
};
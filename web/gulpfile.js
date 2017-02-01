// load all plugins in 'devDependencies' into the variable $
var gulp = require('gulp');

const $ = require('gulp-load-plugins')({
        rename: {
            'gulp-add-src': 'add_src',
            'webpack-stream': 'webpack_stream',
        },
        pattern: ['*'],
        scope: ['devDependencies']
    });

// package vars
const pkg = require('./package.json');

// scss - build the scss to the build folder, including the required paths, and writing out a sourcemap
gulp.task('scss', () => {
    $.fancyLog("-> Compiling scss: " + pkg.paths.dist.css + pkg.vars.scssName);
    return gulp.src(pkg.paths.src.scss + pkg.vars.scssName)
        // .pipe($.plumber({ errorHandler: onError }))
        .pipe($.add_src(pkg.globs.distCss))
        // .pipe($.sourcemaps.init())
        .pipe($.sass({
                includePaths: pkg.paths.scss
            })
            .on('error', $.sass.logError))
        .pipe($.cached('sass_compile'))
        .pipe($.autoprefixer())
        // .pipe($.sourcemaps.write('./'))
        .pipe($.size({ gzip: true, showFiles: true }))
        .pipe(gulp.dest(pkg.paths.dist.css));
});

gulp.task('webpack', () => {
    return gulp.src(pkg.main)
        .pipe($.webpack_stream())
        .pipe(gulp.dest(pkg.paths.dist.js));
});

gulp.task('watch', function() {
  // gulp.watch(kubedir + 'src/_js/*.js', ['scripts']);
  gulp.watch(pkg.paths.src.scss + pkg.vars.scssName, ['scss']);
  gulp.watch('./package.json', ['scss']);
  gulp.watch(pkg.paths.src.js, ['webpack']);
});

gulp.task('default', ['scss', 'webpack', 'watch'])

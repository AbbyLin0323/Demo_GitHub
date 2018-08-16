pipeline {
  agent any
  stages {
    stage('build') {
      parallel {
        stage('build') {
          steps {
            sh 'sh "make"'
            echo 'build...'
          }
        }
        stage('test') {
          steps {
            sh 'sh "make check || true"'
            junit(testResults: '**/target/*.xml', allowEmptyResults: true)
            echo 'test...'
          }
        }
      }
    }
  }
  environment {
    BUILD_ID = '0.1'
  }
}
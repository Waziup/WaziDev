pipeline {
  agent any
  agent {
    docker { image 'ubuntu:latest' }
  }
  stages {
    stage('Build examples') {
      steps {
        sh './make_all.sh'
      }
    }
  }
  post {
    always {
      junit 'tests/report.xml'
    }
    success {
      echo 'Success!'
    }
    failure {
      echo 'Failure!'
    }
    unstable {
      echo 'Unstable'
    }
  }
}
